#include "chunk.h"
#include "memory.h"
#include "value.h"

void init_chunk(Chunk *m_chunk) {
  log_debug("Init chunk : %p", m_chunk);
  m_chunk->count = 0;
  m_chunk->capacity = 0;
  m_chunk->code = NULL;
  m_chunk->lines = NULL;
  init_value_array(&m_chunk->constants);
}

void free_chunk(Chunk *m_chunk) {
  log_debug("free chunk : %p", m_chunk);
  FREE_ARRAY(uint8_t, m_chunk->code, m_chunk->capacity);
  FREE_ARRAY(size_t, m_chunk->lines, m_chunk->capacity);
  free_value_array(&m_chunk->constants);
  init_chunk(m_chunk);
}

void write_chunk(Chunk *m_chunk, uint8_t m_byte, size_t line) {
  log_debug("write chunk : %p, byte: %d", m_chunk, m_byte);

  if (m_chunk->capacity < m_chunk->count + 1) {
    size_t old_capacity = m_chunk->capacity;
    m_chunk->capacity = GROW_CAPACITY(old_capacity);
    m_chunk->code =
        GROW_ARRAY(uint8_t, m_chunk->code, old_capacity, m_chunk->capacity);
    m_chunk->lines =
        GROW_ARRAY(size_t, m_chunk->lines, old_capacity, m_chunk->capacity);
  }

  m_chunk->code[m_chunk->count] = m_byte;
  m_chunk->lines[m_chunk->count] = line;
  m_chunk->count++;
}

size_t add_constant_to_chunk(Chunk *chunk, Value value) {
  write_value_array(&chunk->constants, value);
  return chunk->constants.count - 1;
}
