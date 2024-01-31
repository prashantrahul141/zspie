#include "memory.h"

void *reallocate(void *pointer, size_t old_size, size_t new_size) {
  // free up the allocated memory if new_size is 0.
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  // reallocate and return new pointer.
  return realloc(pointer, new_size);
}
