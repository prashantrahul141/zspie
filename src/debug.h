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
size_t disassemble_instruction(Chunk *m_chunk, size_t m_offset);

/**
 * Prints name and offset of a simple instruction.
 * @param name
 * @param offset
 */
size_t simple_instruction(const char *name, size_t offset);

/**
 * Debug outputs a constant instruction.
 * @param chunk
 * @param offset
 */
size_t constant_instruction(const char *name, Chunk *chunk, size_t offset);

/*
 * Uses to disassemble locals
 */
size_t byte_instruction(const char *name, Chunk *chunk, size_t offset);

/*
 * used to debug jump op codes.
 */
size_t jump_instruction(const char *name, int sign, Chunk *chunk,
                        size_t offset);

#endif // !ZSPIE_DEBUG_H_
