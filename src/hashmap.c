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

static unsigned long hashmap_hash(const char *key) {
  unsigned long hash_value = 0;
  for (size_t i = 0; key[i] != '\0'; i++) {
    hash_value = 31 * hash_value + (unsigned char)key[i];
  }
  return hash_value;
}
void *hashmap_get(struct hashmap *map, const char *key) {
  unsigned long hash = hashmap_hash(key);
  size_t index = hash % map->capacity;

  while (map->entries[index].key != NULL) {
    if (strcmp(key, map->entries[index].key) == 0) {
      return map->entries[index].value;
    }

    index++;
    if (index >= map->capacity) {
      index = 0;
    }
  }
  return NULL;
}
