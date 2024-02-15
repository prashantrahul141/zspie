#ifndef ZSPIE_VM_H_
#define ZSPIE_VM_H_

#include "chunk.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

#define MAX_STACK_SIZE 256

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
  /// stack for the VM.
  Value stack[MAX_STACK_SIZE];
  // pointing at the top of the stack, not at the top value,
  // but at the top most empty value.
  Value *stack_top;
  // global variables
  Table globals;
  // all string objects in hash table.
  Table strings;
  // pointers to the head of dynamic objects created on the heap.
  struct Obj *objects;
} VM;

/*
 * Possible outcomes of our interpreter.
 */
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

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
InterpretResult interpret(const char *source);

/*
 * stack operations for our vm: pushing
 */
void push(Value value);

/*
 * stack operations for our vm: pop
 */
Value pop();

#endif // ZSPIE_VM_H_
