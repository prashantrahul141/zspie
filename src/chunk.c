#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *m_chunk) {
  log_debug("Init chunk : %p", m_chunk);
  m_chunk->capacity = 0;
  m_chunk->code = 0;
  m_chunk->code = NULL;
}

void free_chunk(Chunk *m_chunk) {
  log_debug("free chunk : %p", m_chunk);
  FREE_ARRAY(uint8_t, m_chunk->code, m_chunk->capacity);
  init_chunk(m_chunk);
}

void write_chunk(Chunk *m_chunk, uint8_t m_byte) {
  log_debug("write chunk : %p, byte: %c", m_chunk, m_byte);

  if (m_chunk->capacity < m_chunk->count + 1) {
    size_t old_capacity = m_chunk->capacity;
    m_chunk->capacity = GROW_CAPACITY(old_capacity);
  }

  m_chunk->code[m_chunk->count] = m_byte;
  m_chunk->count++;
}
