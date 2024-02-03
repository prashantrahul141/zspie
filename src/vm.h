#ifndef ZSPIE_VM_H_
#define ZSPIE_VM_H_

#include "chunk.h"
#include <stdint.h>

/*
 * Struct for our vm, it will hold the vm's state.
 */
typedef struct {
  // The chunk our vm is processing.
  Chunk *chunk;
  // instruction pointer right between the chunk array.
  // this will always point to the next instruction which
  // will be executed.
  uint8_t *ip;
} VM;

/*
 * Possible outcomes of our interpreter.
 */
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERRR
} InterpretResult;

/* Initialiser for our vm.
 *
 */
void init_vm();

/* Restores our vm's state.
 *
 */
void free_vm();

/*
 * Wrapper around run, to make it easier to keep state and init execution.
 */
InterpretResult interpret(Chunk *chunk);

#endif // ZSPIE_VM_H_
