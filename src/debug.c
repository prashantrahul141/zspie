#include "debug.h"
#include "chunk.h"
#include <stdint.h>
#include <stdio.h>

size_t disassemble_instruction(Chunk *m_chunk, size_t m_offset) {
  printf("%zu", m_offset);
  uint8_t instruction = m_chunk->code[m_offset];
  log_debug("disassembling instruction from chunk : %p, offset : %d, "
            "instruction : %d",
            m_chunk, m_offset, instruction);
  switch (instruction) {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", m_offset);
  default:
    printf("unknown instruction %hhu", instruction);
    return m_offset + 1;
  }
}

void disassemble_chunk(Chunk *m_chunk, const char *name) {
  printf("== %s ==", name);
  log_debug("disassembling chunk : %p, name : %s", m_chunk, name);

  for (size_t offset = 0; offset < m_chunk->count;) {
    offset = disassemble_instruction(m_chunk, offset);
  }
}
