#ifndef HASHMAP_H
#define HASHMAP_H

struct hashmap;

struct hashmap *hashmap_create(void (*free_value)(void *value));

void hashmap_destroy(struct hashmap *map);

const char *hashmap_put(struct hashmap *map, char *key, char *value);

void *hashmap_get(struct hashmap *map, const char *key);

#endif
