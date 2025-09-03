#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

struct hashmap_entry {
  const char *key;
  void *value;
};

struct hashmap {
  struct hashmap_entry *entries;
  size_t capacity;
  size_t length;
  void (*free_value)(void *value);
};

struct hashmap *hashmap_create(void (*free_value)(void *value)) {
  struct hashmap *map = malloc(sizeof(struct hashmap));
  if (map == NULL) {
    return NULL;
  }
  map->length = 0;
  map->capacity = INITIAL_CAPACITY;
  map->free_value = free_value;

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
    if (map->free_value != NULL) {
      map->free_value(map->entries[i].value);
    }
  }

  free(map->entries);
  free(map);
}

static bool hashmap_expand(struct hashmap *map) {
  size_t new_capacity = map->capacity * 2;

  // handle overflow
  if (new_capacity < map->capacity) {
    return false;
  }

  struct hashmap_entry *new_entries =
      calloc(new_capacity, sizeof(struct hashmap_entry));
  if (new_entries == NULL) {
    return false;
  }

  for (size_t i = 0; i < map->capacity; i++) {
    struct hashmap_entry entry = map->entries[i];
    if (entry.key != NULL) {
      // hashmap_set_entry(new_entries, new_capacity, entry.key, entry.value,
      //                   NULL);
    }
  }

  free(map->entries);
  map->entries = new_entries;
  map->capacity = new_capacity;
  return true;
}

static unsigned long hashmap_hash(const char *key) {
  unsigned long hash_value = 0;
  for (size_t i = 0; key[i] != '\0'; i++) {
    hash_value = 31 * hash_value + (unsigned char)key[i];
  }
  return hash_value;
}

static const char *hashmap_set_entry(struct hashmap_entry *entries,
                                     size_t capacity, const char *key,
                                     void *value, size_t *map_length) {

  unsigned long hash = hashmap_hash(key);
  size_t index = hash % capacity;

  while (entries[index].key != NULL) {

    if (strcmp(key, entries[index].key) == 0) {
      entries[index].value = value;
      return entries[index].key;
    }

    index++;
    if (index >= capacity) {
      index = 0;
    }
  }

  if (map_length != NULL) {
    key = strdup(key);
    if (key == NULL) {
      return NULL;
    }
    (*map_length)++;
  }
  entries[index].key = (char *)key;
  entries[index].value = value;
  return key;
}

const char *hashmap_put(struct hashmap *map, char *key, char *value) {
  if (value == NULL) {
    return NULL;
  }

  unsigned long index;

  if (map->length >= map->capacity * 3 / 4) {
    if (!hashmap_expand(map)) {
      return NULL;
    }
  }

  do {
    index = hashmap_hash(key) % map->capacity;
  } while (map->entries[index].key != NULL);

  return hashmap_set_entry(map->entries, map->capacity, key, value,
                           &map->length);
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

size_t hashmap_get_length(struct hashmap *map) { return map->length; }
