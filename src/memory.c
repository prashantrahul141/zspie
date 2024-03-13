#include "memory.h"
#include "chunk.h"
#include "object.h"
#include "vm.h"

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

static void free_object(Obj *obj) {
  switch (obj->type) {
  case OBJ_FUNCTION: {
    ObjFunction *function = (ObjFunction *)obj;
    free_chunk(&function->chunk);
    FREE(ObjFunction, obj);
    break;
  }
  case OBJ_NATIVE: {
    FREE(ObjNative, obj);
    break;
  }
  case OBJ_STRING: {
    ObjString *obj_string = (ObjString *)obj;
    FREE_ARRAY(char, obj_string->chars, obj_string->length);
    FREE(ObjString, obj_string);
    break;
  }
  }
}

void free_objects() {
  Obj *obj = vm.objects;
  while (obj != NULL) {
    Obj *next = obj->next;
    free_object(obj);
    obj = next;
  }
}
