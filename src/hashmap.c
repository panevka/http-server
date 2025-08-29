#include <stddef.h>
#include <stdlib.h>

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

struct hashmap *hashmap_create(void) {
  struct hashmap *map = malloc(sizeof(struct hashmap));
  if (map == NULL) {
    return NULL;
  }
  map->length = 0;
  map->capacity = INITIAL_CAPACITY;

  map->entries = calloc(map->capacity, sizeof(struct hashmap_entry));
  if (map->entries == NULL) {
    free(map);
    return NULL;
  }

  return map;
}

void hashmap_destroy(struct hashmap *map) {
  for (size_t i = 0; i < map->capacity; i++) {
    free((void *)map->entries[i].key);
  }

  free(map->entries);
  free(map);
}
