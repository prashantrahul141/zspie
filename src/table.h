#ifndef ZSPIE_TABLE_H_
#define ZSPIE_TABLE_H_

#include "common.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <stdint.h>

#define TABLE_MAX_LOAD 0.75

/*
 * One entry in the table.
 */
typedef struct {
  ObjString *key;
  Value value;
} Entry;

/*
 * The Hash table for ZSPIE.
 */
typedef struct {
  size_t count;
  size_t capacity;
  Entry *entries;
} Table;

/*
 * Initialises a table's fields.
 */
void init_table(Table *table);

/*
 * Free's all heap memory of a table.
 */
void free_table(Table *table);

/*
 * Sets a table in the table.
 */
bool table_set(Table *table, ObjString *key, Value value);

/*
 * Copies all table entries from one table to another.
 */
void table_add_all(Table *from, Table *to);

/*
 * retrives an entry from the table. return true or false on search result
 */
bool table_get(Table *table, ObjString *key, Value *value);

/*
 * deletes an entry in the table.
 */
bool table_delete(Table *table, ObjString *key);

/*
 * finds a key in the table.
 */
ObjString *table_find_string(Table *table, const char *chars, size_t length,
                             uint32_t hash);

#endif // !ZSPIE_TABLE_H_
