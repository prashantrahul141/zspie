#ifndef ZSPIE_CHUNK_H_
#define ZSPIE_CHUNK_H_

#include "common.h"

/** Top level enum which stores variants for all types of operations possible in
 * the VM.
 */
typedef enum {
  OP_RETURN,
} OpCode;

/** Dynamic array implementation.
 */
typedef struct {
  size_t capacity;
  size_t count;
  uint8_t *code;
} Chunk;

/** Initialises a new Chunk.
 * @param chunk Pointer to the chunk to initialise.
 */
void init_chunk(Chunk *m_chunk);

/** Deallocates a chunk
 * @param chunk Pointer to the chunk to deallocate.
 */
void free_chunk(Chunk *m_chunk);

/**
 * Write data to a chunk, increases its size if capacity is not enough.
 * @param Chunk pointer to the chunk to write.
 * @param byte data to write
 */
void write_chunk(Chunk *m_chunk, uint8_t m_byte);

#endif // ZSPIE_CHUNK_H_
