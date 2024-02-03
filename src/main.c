#include "chunk.h"
#include "debug.h"
#include "external/log.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  log_set_quiet(false);
  init_vm();

  Chunk chunk;
  init_chunk(&chunk);

  // op constant : 1
  // push 1 to stack.
  int constant = add_constant_to_chunk(&chunk, 1);
  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);

  // op constant : 3
  // push 3 to stack.
  constant = add_constant_to_chunk(&chunk, 3);
  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);

  // op add
  // pops 1 and 3 and pushes 1+3 to stack.
  write_chunk(&chunk, OP_ADD, 1);

  // op constant : 5
  // pushes 5 to stack.
  constant = add_constant_to_chunk(&chunk, 5);
  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);

  // op divide
  // pops 1+3 and 5 and pushes 1+3/5 to stack.
  write_chunk(&chunk, OP_DIVIDE, 1);

  // op negate.
  // pops 1+3/5 and pushes -(1+3/5 to stack).
  write_chunk(&chunk, OP_NEGATE, 1);

  // prints top most value in the stack.
  write_chunk(&chunk, OP_RETURN, 1);

  disassemble_chunk(&chunk, "Test chunk");
  interpret(&chunk);

  free_vm();
  free_chunk(&chunk);

  return EXIT_SUCCESS;
}
