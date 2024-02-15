#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "external/log.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Global, static VM object for our interpreter.
 */
VM vm;

// resets vm's stack.
void reset_vm_stack() { vm.stack_top = vm.stack; }

/*
 * Reports a runtime error
 */
static void runtime_error(const char *format, ...) {
  // report error.
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  log_error(format, args);
  va_end(args);
  fputs("\n", stderr);

  // the instruction which caused runtime error.
  size_t instruction = vm.ip - vm.chunk->code - 1;
  // line number of that instruction.
  size_t line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %zu] in script\n", line);
  log_error("[line %zu] in script\n", line);
  reset_vm_stack();
}

void init_vm() {
  reset_vm_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);
}

void free_vm() {
  free_table(&vm.globals);
  free_table(&vm.strings);
  free_objects();
}

void push(Value value) {
  log_trace("pushing value=%lf to stack.");
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  log_trace("poping value=%lf from stack.", *vm.stack_top);
  return *vm.stack_top;
}

Value peek(size_t distance) { return vm.stack_top[-1 - distance]; }

bool is_falsey(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    return !AS_BOOL(value);
  case VAL_NULL:
    return false;
  case VAL_NUMBER:
    return AS_NUMBER(value) != 0;
  case VAL_OBJ:
    return true;
  }

  return false;
}

void concatenate() {
  ObjString *a = AS_STRING(pop());
  ObjString *b = AS_STRING(pop());

  size_t new_length = a->length + b->length;
  char *new_chars = ALLOCATE(char, new_length + 1);
  memcpy(new_chars, a->chars, a->length);
  memcpy(new_chars + a->length, b->chars, b->length);
  new_chars[new_length] = '\0';

  ObjString *new_obj = take_string(new_chars, new_length);
  push(OBJ_VAL(new_obj));
}

/*
 * Heart of our interpreter execution logic.
 */
static InterpretResult run() {
  // reads the current next instruction and increments the ip.
#define READ_BYTE() (*vm.ip++)

// reads next constant and converts it to strings.
#define READ_STRING() AS_STRING(READ_CONSTANT())

// reads a constant from the chunk
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

// macros for solving binary operations
#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtime_error("Operands must be number.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(value_type(a op b));                                                  \
  } while (false)

  // previous instruction which ran.
  uint8_t instruction = 0;
  // goes forever.
  while (true) {
    log_trace("current state of the stack:");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      log_trace("[ type=%d, value=%d ]", slot->type, slot->as);
    }
    log_trace("previous instruction=%d", instruction);

    switch (instruction = READ_BYTE()) {
    // op_constant instruction.
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }

    case OP_NULL: {
      push(NULL_VAL);
      break;
    }

    case OP_TRUE: {
      push(BOOL_VAL(true));
      break;
    }

    case OP_FALSE: {
      push(BOOL_VAL(false));
      break;
    }

    case OP_POP: {
      pop();
      break;
    }

    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      table_set(&vm.globals, name, peek(0));
      pop();
      break;
    }

    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(values_equal(a, b)));
      break;
    }

    case OP_GREATER: {
      BINARY_OP(BOOL_VAL, >);
      break;
    }

    case OP_LESS: {
      BINARY_OP(BOOL_VAL, <);
      break;
    }

    // binary operation +
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double a = AS_NUMBER(pop());
        double b = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtime_error("Operands must be two strings or two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
      BINARY_OP(NUMBER_VAL, +);
      break;

    // binary operation -
    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;

    // binary operation *
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;

    // binary operation /
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;

      // not operation !
    case OP_NOT:
      push(BOOL_VAL(is_falsey(pop())));
      break;

    // op_negate instruction.
    case OP_NEGATE: {
      if (!IS_NUMBER(peek(0))) {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    }

    case OP_PRINT: {
      print_value(pop());
      printf("\n");
      break;
    }

      // op_return instruction.
    case OP_RETURN: {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

/*
 * Takes source string, compiles it and run its.
 * @param source pointer to source strings.
 * @returns InterpretResult
 */
InterpretResult interpret(const char *source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
  InterpretResult result = run();

  free_chunk(&chunk);
  return result;
}
