#include "object.h"
#include "chunk.h"
#include "memory.h"
#include "stdio.h"
#include "table.h"
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

struct ObjString *allocate_string(char *chars, size_t length, uint32_t hash) {
  ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  table_set(&vm.strings, string, NULL_VAL);
  return string;
}

// THE hash function in zspie
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
uint32_t hash_string(const char *key, size_t length) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }

  return hash;
}

// allocates memory for new function object.
ObjFunction *new_function() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  init_chunk(&function->chunk);
  return function;
}

// create new C native object.
ObjNative *new_native(NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjString *take_string(char *chars, size_t length) {
  uint32_t hash = hash_string(chars, length);

  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return allocate_string(chars, length, hash);
}

ObjString *copy_string(const char *chars, size_t length) {
  uint32_t hash = hash_string(chars, length);
  ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }

  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length, hash);
}

void print_function(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("\"%s\"", AS_CSTRING(value));
    break;

  case OBJ_FUNCTION:
    print_function(AS_FUNCTION(value));
    break;

  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  }
}
