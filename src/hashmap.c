#include <stddef.h>

#define INITIAL_CAPACITY 16

struct hashmap_entry {
  const char *key;
  void *value;
};

struct hashmap {
  struct hashmap_entry *entries;
  size_t capacity;
  size_t length;
};
