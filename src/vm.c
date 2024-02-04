#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "external/log.h"
#include "value.h"
#include <stdint.h>

/*
 * Global, static VM object for our interpreter.
 */
VM vm;

// resets vm's stack.
void reset_vm_stack() { vm.stack_top = vm.stack; }

void init_vm() { reset_vm_stack(); }

void free_vm() {}

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

/*
 * Heart of our interpreter execution logic.
 */
static InterpretResult run() {
  // reads the current next instruction and increments the ip.
#define READ_BYTE() (*vm.ip++)
// reads a constant from the chunk
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
// macros for solving binary operations
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double b = pop();                                                          \
    double a = pop();                                                          \
    push(a op b);                                                              \
  } while (false)

  // goes forever.
  while (true) {
    // previous instruction which ran.
    uint8_t instruction;

    log_trace("current state of the stack:");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      log_trace("[ %g ]", *slot);
    }
    log_trace("previous instruction=%d", instruction);

    switch (instruction = READ_BYTE()) {
    // op_constant instruction.
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }

    // binary operation +
    case OP_ADD:
      BINARY_OP(+);
      break;

    // binary operation -
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;

    // binary operation *
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;

    // binary operation /
    case OP_DIVIDE:
      BINARY_OP(/);
      break;

    // op_negate instruction.
    case OP_NEGATE: {
      push(-pop());
      break;
    }

      // op_return instruction.
    case OP_RETURN: {
      print_value(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
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
