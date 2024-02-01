#include "memory.h"

void *reallocate(void *m_pointer, size_t m_old_size, size_t m_new_size) {
  log_trace("Called reallocate with pointer: %d, old_size: %d, new_size : %d",
            m_pointer, m_old_size, m_new_size);
  // free up the allocated memory if new_size is 0.
  if (m_new_size == 0) {
    free(m_pointer);
    return NULL;
  }

  // reallocate and return new pointer.
  void *new_allocation = realloc(m_pointer, m_new_size);
  if (new_allocation == NULL) {
    exit(1);
  }
  return new_allocation;
}
