#ifndef ZSPIE_MEMORY_H_
#define ZSPIE_MEMORY_H_

#include "common.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count)                        \
  (type *)reallocate(pointer, sizeof(type) * (old_count),                      \
                     sizeof(type) * (new_count))

/** Reallocates memory of new_count size, and frees old allocated memory.
 * @param chunk Pointer to the chunk to initialise.
 */
void *reallocate(void *, size_t, size_t);

#endif // ZSPIE_MEMORY_H_
