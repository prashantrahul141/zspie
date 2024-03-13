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
#include <time.h>

static Value clock_native(int argCount, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/*
 * Global, static VM object for our interpreter.
 */
VM vm;

// resets vm's stack.
void reset_vm_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
}

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

  CallFrame *frame = &vm.frames[vm.frame_count - 1];
  size_t instruction = frame->ip - frame->function->chunk.code - 1;
  size_t line = frame->function->chunk.lines[instruction];

  fprintf(stderr, "[line %zu] in script\n", line);
  log_error("[line %zu] in script\n", line);

  for (int i = vm.frame_count - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %zu] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_vm_stack();
}

/*
 * Defines Native function
 */
static void define_native(const char *name, NativeFn function) {
  push(OBJ_VAL(copy_string(name, (int)strlen(name))));
  push(OBJ_VAL(new_native(function)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void init_vm() {
  reset_vm_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);
  define_native("clock", clock_native);
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

static bool call(ObjFunction *function, int args_count) {
  if (args_count != function->arity) {
    runtime_error("Expected %d arguments got %d.", function->arity, args_count);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frame_count++];
  frame->function = function;
  frame->ip = function->chunk.code;
  frame->slots = vm.stack_top - args_count - 1;
  return true;
}

static bool call_value(Value callee, int args_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_FUNCTION:
      return call(AS_FUNCTION(callee), args_count);

    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(args_count, vm.stack_top - args_count);
      vm.stack_top -= args_count + 1;
      push(result);
      return true;
    }

    default:
      break;
    }
  }

  runtime_error("Can only call functions.");
  return false;
}

bool is_falsey(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    // bools are their values
    return !AS_BOOL(value);

  case VAL_NULL:
    // nulls are false
    return false;

  case VAL_NUMBER:
    // all non zero numbers are true and 0 is false.
    return AS_NUMBER(value) == 0;

  case VAL_OBJ:
    // all objects are true.
    return false;
  default:
    // everything else is false just in case.
    return false;
  }
}

void concatenate() {

  ObjString *b = AS_STRING(pop());
  ObjString *a = AS_STRING(pop());

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
  // current stack frame
  CallFrame *frame = &vm.frames[vm.frame_count - 1];

// reads the current next instruction and increments the ip.
#define READ_BYTE() (*frame->ip++)

// reads 16 bits.
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

// reads a constant from the chunk
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])

// reads next constant and converts it to strings.
#define READ_STRING() AS_STRING(READ_CONSTANT())

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

    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }

    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }

    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (table_set(&vm.globals, name, peek(0))) {
        table_delete(&vm.globals, name);
        runtime_error("Undefined variable '%s'", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }

    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!table_get(&vm.globals, name, &value)) {
        runtime_error("Undefined variable '%s'", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
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
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(0))) {
        frame->ip += offset;
      }
      break;
    }

    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }

    case OP_CALL: {
      int args_count = READ_BYTE();
      if (!call_value(peek(args_count), args_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }

      // op_return instruction.
    case OP_RETURN: {
      Value result = pop();
      vm.frame_count--;
      if (vm.frame_count == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stack_top = frame->slots;
      push(result);
      frame = &vm.frames[vm.frame_count - 1];
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
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
  clock_t before_com = clock();

  ObjFunction *function = compile(source);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJ_VAL(function));

  log_info("Compilation finished. Starting execution.\n\n");
  clock_t before_exec = clock();

  call(function, 0);
  InterpretResult result = run();

  log_info("Compilation took : %ld", clock() - before_com);
  log_info("Execution took : %ld", clock() - before_exec);

  return result;
}
