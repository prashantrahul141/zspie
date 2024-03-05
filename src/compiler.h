#ifndef ZSPIE_COMPILER_H_
#define ZSPIE_COMPILER_H_

#include "chunk.h"
#include "object.h"

/*
 * Compiles source strings into tokens.
 */
ObjFunction *compile(const char *source);

#endif // !ZSPIE_COMPILER_H_
