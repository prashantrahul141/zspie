#include "debug.h"
#include "chunk.h"
#include "value.h"

size_t disassemble_instruction(Chunk *chunk, size_t offset) {
  printf("%04zu ", offset);
  uint8_t instruction = chunk->code[offset];

  log_debug("disassembling instruction from chunk : %p, offset : %d, "
            "instruction : %d",
            chunk, offset, instruction);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4zu ", chunk->lines[offset]);
  }

  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_NULL:
    return simple_instruction("OP_NULL", offset);
  case OP_TRUE:
    return simple_instruction("OP_TRUE", offset);
  case OP_FALSE:
    return simple_instruction("OP_FALSE", offset);
  case OP_EQUAL:
    return simple_instruction("OP_EQUAL", offset);
  case OP_LESS:
    return simple_instruction("OP_LESS", offset);
  case OP_GREATER:
    return simple_instruction("OP_GREATER", offset);
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simple_instruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simple_instruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simple_instruction("OP_DIVIDE", offset);
  case OP_NOT:
    return simple_instruction("OP_NOT", offset);
  case OP_NEGATE:
    return simple_instruction("OP_NEGATE", offset);
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    printf("unknown instruction %hhu", instruction);
    return offset + 1;
  }
}

void disassemble_chunk(Chunk *m_chunk, const char *name) {
  printf("== %s ==\n", name);
  printf("IN      L I                CI  CV\n");
  log_debug("disassembling chunk : %p, name : %s", m_chunk, name);

  for (size_t offset = 0; offset < m_chunk->count;) {
    offset = disassemble_instruction(m_chunk, offset);
  }
}

size_t simple_instruction(const char *name, size_t offset) {
  printf("%s\n", name);
  return offset + 1;
}

size_t constant_instruction(const char *name, Chunk *chunk, size_t offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%s      %d   ", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("\n");
  return offset + 2;
}
