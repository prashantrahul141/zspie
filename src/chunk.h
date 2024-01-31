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
void initChunk(Chunk *);

/**
 * Write data to a chunk, increases its size if capacity is not enough.
 * @param Chunk pointer to the chunk to write.
 * @param byte data to write
 */
void writeChunk(Chunk *, uint8_t);

#endif // ZSPIE_CHUNK_H_
