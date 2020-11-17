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


#define HASH_TABLE_SIZE 307


struct map_item *map_item_create(char *id, enum types type, void *value);

struct map_item *map_item_create_with_sub(char *id, enum types type, void *value, void *subvalue);

struct map_item **hashmap_create();

void hashmap_empty(struct map_item **hashmap, bool free_ids);

int hash(char *id, int m);

void hashmap_insert(struct map_item **hashmap, struct map_item *item);

struct map_item *hashmap_search(struct map_item **hashmap, char *id, enum types type);

struct map_item *hashmap_search_with_sub(struct map_item **hashmap, char *id, enum types type, void *sub);


#endif
