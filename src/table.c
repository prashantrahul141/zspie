#include "table.h"
#include "memory.h"

void init_table(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_table(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}

static Entry *find_entry(Entry *entries, size_t capacity, ObjString *key) {
  uint32_t index = key->hash % capacity;
  while (true) {
    Entry *entry = &entries[index];
    if (entry->key == key || entry->key == NULL) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
};

