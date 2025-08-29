#ifndef HASHMAP_H
#define HASHMAP_H

struct hashmap;

struct hashmap *hashmap_create(void);

const char *hashmap_put(struct hashmap *map, char *key, char *value);

#endif
