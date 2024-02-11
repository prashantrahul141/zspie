#ifndef ZSPIE_OBJECT_H_
#define ZSPIE_OBJECT_H_

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) isObjectType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj *next;
};

struct ObjString {
  Obj obj;
  size_t length;
  char *chars;
};

ObjString *take_string(char *chars, size_t length);
ObjString *copy_string(const char *chars, size_t length);

static inline bool isObjectType(Value value, ObjType obj_type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == obj_type;
}

void print_object(Value value);

#endif // !ZSPIE_OBJECT_H_
