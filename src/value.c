#include "value.h"
#include "external/log.h"
#include "memory.h"
#include "object.h"
#include <stdio.h>
#include <string.h>

bool values_equal(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NULL:
    return true;
  case VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case VAL_OBJ: {
    ObjString *string_a = AS_STRING(a);
    ObjString *string_b = AS_STRING(b);
    return string_a->length == string_b->length &&
           memcmp(string_a->chars, string_b->chars, string_a->length) == 0;
  }
  default:
    return false; // unreachable.
  }
}

void init_value_array(ValueArray *value_array) {
  log_trace("initalizing value array *value_array=%p", value_array);
  value_array->capacity = 0;
  value_array->count = 0;
  value_array->values = NULL;
}

void write_value_array(ValueArray *value_array, Value value) {
  log_trace("writing to value array *value_array=%p, value=%lf", value_array,
            value);

  if (value_array->capacity < value_array->count + 1) {
    size_t old_capacity = value_array->capacity;
    value_array->capacity = GROW_CAPACITY(old_capacity);
    log_trace("growing value array since there was not enough space left "
              "new_capcity=%d",
              value_array->capacity);
    value_array->values = GROW_ARRAY(Value, value_array->values, old_capacity,

                                     value_array->capacity);
  }

  value_array->values[value_array->count] = value;
  value_array->count++;
}

void free_value_array(ValueArray *value_array) {
  log_trace("freeing value array");
  FREE_ARRAY(Value, value_array->values, value_array->capacity);
  init_value_array(value_array);
}

void print_value(Value value) {
  switch (value.type) {

  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;

  case VAL_NULL:
    printf("null");
    break;

  case VAL_NUMBER:
    printf("'%g'", AS_NUMBER(value));
    break;

  case VAL_OBJ:
    print_object(value);
    break;
  }
}
