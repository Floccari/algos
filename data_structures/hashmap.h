#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


enum types {
    STATE,
    TRANSITION,
    AUTOMATON,
    LINK,
    EVENT,
    LABEL,
    CONTEXT
};

struct map_item {
    char *id;
    enum types type;
    // 4 bytes
    void *value;
    void *subvalue;
    struct map_item *next;
};

struct hashmap {
    struct map_item **buffer;
    int size;
    // 4 bytes
};

#define HASH_TABLE_SIZE 307


struct map_item *map_item_create(char *id, enum types type, void *value);

struct map_item *map_item_create_with_sub(char *id, enum types type, void *value, void *subvalue);

struct hashmap *hashmap_create(int size);

void hashmap_empty(struct hashmap *hashmap, bool free_ids);

void hashmap_destroy(struct hashmap *hashmap);

int hash(char *id, int m);

void hashmap_insert(struct hashmap *hashmap, struct map_item *item);

struct map_item *hashmap_search(struct  hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_search_with_sub(struct hashmap *hashmap, char *id, enum types type, void *sub);

struct map_item *hashmap_search_and_remove(struct hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_search_with_sub_and_remove(struct hashmap *hashmap, char *id, enum types type, void *sub);

#endif
