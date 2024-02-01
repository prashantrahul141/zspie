#ifndef ZSPIE_MEMORY_H_
#define ZSPIE_MEMORY_H_

#include "common.h"

// gives new grow capacity based on current capacity.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Increases array size, wrapper around reallocate.
#define GROW_ARRAY(type, pointer, old_count, new_count)                        \
  (type *)reallocate(pointer, sizeof(type) * (old_count),                      \
                     sizeof(type) * (new_count))

// Frees array, wrapper around reallocate.
#define FREE_ARRAY(type, pointer, old_capacity)                                \
  reallocate(pointer, sizeof(type) * (old_capacity), 0)

/** Reallocates memory of new_count size, and frees old allocated memory.
 * @param chunk Pointer to the chunk to initialise.
 */
void *reallocate(void *m_pointer, size_t m_old_capacity, size_t m_new_capacity);

#endif // ZSPIE_MEMORY_H_
