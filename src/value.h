#ifndef ZSPIE_VALUE_H_
#define ZSPIE_VALUE_H_

#include "common.h"

/**
 * Our abstraction layer of c's double.
 */
typedef double Value;

/** ValueArray
 * This holds a dynamic array of values present in a chunk of instructions.
 */
typedef struct {
  // total capacity of the array.
  size_t capacity;
  // count of storage currently filled.
  size_t count;
  // pointer to the actual array in the memory.
  Value *values;

} ValueArray;

/*
 * Initialises a new value array.
 * @param value_array - pointer to the ValueArray.
 */
void init_value_array(ValueArray *value_array);

/*
 * Writes a value to the value array.
 * @param value_array - pointer to the ValueArray.
 * @param value - `value` data to write to the value array.
 */
void write_value_array(ValueArray *value_array, Value value);

/*
 * frees a ValueArray.
 * @param value_array - pointer to the ValueArray.
 */
void free_value_array(ValueArray *value_array);

/*
 * prints a Value constant.
 * @param value - Value constant.
 */
void print_value(Value value);

#endif // !ZSPIE_VALUE_H_
