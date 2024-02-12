#ifndef ZSPIE_TABLE_H_
#define ZSPIE_TABLE_H_

#include "common.h"
#include "memory.h"
#include "object.h"
#include <stdint.h>

#define TABLE_MAX_LOAD 0.75

typedef struct {
  ObjString *key;
  Value value;
} Entry;

typedef struct {
  size_t count;
  size_t capacity;
  Entry *entries;

} Table;

void init_table(Table *table);
void free_table(Table *table);
bool table_set(Table *table, ObjString *key, Value value);

#endif // !ZSPIE_TABLE_H_
