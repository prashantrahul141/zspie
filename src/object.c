#include "object.h"
#include "memory.h"
#include "stdio.h"
#include "value.h"
#include "vm.h"
#include <string.h>

#define ALLOCATE_OBJ(type, obj_type)                                           \
  (type *)allocate_object(sizeof(type), obj_type)

static Obj *allocate_object(size_t size, ObjType obj_type) {
  Obj *obj = (Obj *)reallocate(NULL, 0, size);
  obj->type = obj_type;
  obj->next = vm.objects;
  vm.objects = obj;
  return obj;
}

struct ObjString *allocate_string(char *chars, size_t length) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

ObjString *take_string(char *chars, size_t length) {
  return allocate_string(chars, length);
}

ObjString *copy_string(const char *chars, size_t length) {
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("\"%s\"", AS_CSTRING(value));
    break;
  }
}
