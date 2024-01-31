#include "chunk.h"
#include "memory.h"

/** Sets capactiy and count of a chunk to 0, sets pointer to the data to NULL.
 */
void initChunk(Chunk *m_chunk) {
  m_chunk->capacity = 0;
  m_chunk->code = 0;
  m_chunk->code = NULL;
}

void writeChunk(Chunk *chunk, uint8_t byte) {
  if (chunk->capacity < chunk->count + 1) {
    size_t old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;
}
