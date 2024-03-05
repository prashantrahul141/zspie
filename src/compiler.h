#ifndef ZSPIE_COMPILER_H_
#define ZSPIE_COMPILER_H_

#include "chunk.h"
#include "object.h"

/*
 * Compiles source strings into tokens.
 */
ObjFunction *compile(const char *source, Chunk *chunk);

#endif // !ZSPIE_COMPILER_H_
