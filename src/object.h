#ifndef ZSPIE_OBJECT_H_
#define ZSPIE_OBJECT_H_

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjectType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjectType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjectType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;
};

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int arg_count, Value *args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

struct ObjString {
  Obj obj;
  size_t length;
  char *chars;
  uint32_t hash;
};

ObjFunction *new_function();
ObjNative *new_native(NativeFn function);
ObjString *take_string(char *chars, size_t length);
ObjString *copy_string(const char *chars, size_t length);

static inline bool isObjectType(Value value, ObjType obj_type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == obj_type;
}

void print_object(Value value);

#endif // !ZSPIE_OBJECT_H_
