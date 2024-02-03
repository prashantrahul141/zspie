#include "vm.h"
#include "chunk.h"
#include "value.h"
#include <stdint.h>

/*
 * Global, static VM object for our interpreter.
 */
VM vm;

void init_vm() {}

void free_vm() {}

/*
 * Heart of our interpreter execution logic.
 */
static InterpretResult run() {
  // reads the current next instruction and increments the ip.
#define READ_BYTE() (*vm.ip++)
// reads a constant from the chunk
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  // goes forever.
  while (true) {
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    // op_constant instruction.
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      print_value(constant);
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
}

InterpretResult interpret(Chunk *chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}
