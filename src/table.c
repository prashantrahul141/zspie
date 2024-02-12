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

static void adjust_capacity(Table *table, size_t capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);

  // resetting new entries.
  for (size_t i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // copying old entries to new location.
  for (size_t i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL) {
      continue;
    }

    Entry *destination = find_entry(entries, capacity, entry->key);
    destination->key = entry->key;
    destination->value = entry->value;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

