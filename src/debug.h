#ifndef ZSPIE_DEBUG_H_
#define ZSPIE_DEBUG_H_

#include "chunk.h"

/**
 * disassembles chunk, for debug purposes only.
 * @param Pointer to the chunk.
 * @param Name of the chunk.
 */
void disassemble_chunk(Chunk *m_chunk, const char *m_name);

/**
 * disassembles instruction, for debug purposes only.
 * @param Pointer to the chunk.
 * @param offset of the instruction in chunk.
 */
size_t isassemble_instruction(Chunk *m_chunk, size_t m_offset);

#endif // !ZSPIE_DEBUG_H_
