#include "value.h"
#include "external/log.h"
#include "memory.h"

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

void print_value(Value value) { printf("'%g'", value); }
