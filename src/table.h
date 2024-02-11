#ifndef ZSPIE_TABLE_H_
#define ZSPIE_TABLE_H_

#include "common.h"
#include "value.h"

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

#endif // !ZSPIE_TABLE_H_
