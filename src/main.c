#include "chunk.h"
#include "debug.h"
#include "external/log.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  log_set_quiet(true);
  init_vm();

  Chunk chunk;
  init_chunk(&chunk);

  int constant = add_constant_to_chunk(&chunk, 2.12);

  write_chunk(&chunk, OP_CONSTANT, 1);
  write_chunk(&chunk, constant, 1);
  write_chunk(&chunk, OP_RETURN, 1);

  disassemble_chunk(&chunk, "Test chunk");
  interpret(&chunk);

  free_vm();
  free_chunk(&chunk);

  return EXIT_SUCCESS;
}
