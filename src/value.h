#ifndef ZSPIE_VALUE_H_
#define ZSPIE_VALUE_H_

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

/*
 * All builtin types of zspie.
 */
typedef enum {
  // booleans
  VAL_BOOL,
  // nulls
  VAL_NULL,
  // numbers
  VAL_NUMBER,
  // objects like strings, functions, classes.
  VAL_OBJ,
} ValueType;

/**
 * Our abstraction layer for values mapped to c types.
 */
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as;
} Value;

bool values_equal(Value a, Value b);

// Some helper macros to convert C values to Zspie's Values.
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}}) // for booleans
#define NULL_VAL ((Value){VAL_NULL, {.number = 0}})             // for nulls
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}}) // for number
#define OBJ_VAL(object)                                                        \
  ((Value){VAL_OBJ, {.obj = (Obj *)object}}) // for objects.

// Some helpers to unpack Zspie's value into C types.
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

// helpers macros to check to check type of a Value.
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_NULL(value) ((value).type == VAL_NULL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

/* ValueArray
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
