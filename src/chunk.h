#ifndef ZSPIE_CHUNK_H_
#define ZSPIE_CHUNK_H_

#include "common.h"
#include "value.h"

/** Top level enum which stores variants for all types of operations possible in
 * the VM.
 */
typedef enum {
  OP_CONSTANT,
  OP_NULL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_SET_LOCAL,
  OP_GET_LOCAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_GLOBAL,
  OP_JUMP_IF_FALSE,
  OP_RETURN,
} OpCode;

/** Dynamic array implementation.
 */
typedef struct {
  // total current storing capacity of the array.
  size_t capacity;
  // actual used storage.
  size_t count;
  // pointer to the op codes allocated memory.
  uint8_t *code;
  // constants used in the chunk.
  ValueArray constants;
  // lines number of each instruction in the same index as the code's array.
  size_t *lines;

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
void write_chunk(Chunk *m_chunk, uint8_t m_byte, size_t line);

/* Adds a constant value to the values array in a chunk.
 * @param chunk pointer to the chunk.
 * @parma value the value to write.
 */
size_t add_constant_to_chunk(Chunk *chunk, Value value);

#endif // ZSPIE_CHUNK_H_
