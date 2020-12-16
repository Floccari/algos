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
    size_t nelem;
};


struct map_item *map_item_create(char *id, enum types type, void *value);

struct map_item *map_item_create_with_sub(char *id, enum types type, void *value, void *subvalue);

// allocate both hashmap and buffer
struct hashmap *hashmap_create(size_t nelem);

void hashmap_buffer_allocate(struct hashmap *hashmap, size_t nelem);

void hashmap_buffer_deallocate(struct hashmap *hashmap);

void hashmap_empty(struct hashmap *hashmap, bool free_ids);

// free both hashmap and buffer
void hashmap_destroy(struct hashmap *hashmap);

void hashmap_insert(struct hashmap *hashmap, struct map_item *item);

/*** compare pointers ***/

struct map_item *hashmap_search(struct hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_search_with_sub(struct hashmap *hashmap, char *id, enum types type, void *sub);

struct map_item *hashmap_search_and_remove(struct hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_search_with_sub_and_remove(struct hashmap *hashmap, char *id, enum types type, void *sub);

/*** compare strings ***/

struct map_item *hashmap_strcmp_search(struct hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_strcmp_search_with_sub(struct hashmap *hashmap, char *id, enum types type, void *sub);

struct map_item *hashmap_strcmp_search_and_remove(struct hashmap *hashmap, char *id, enum types type);

struct map_item *hashmap_strcmp_search_with_sub_and_remove(struct hashmap *hashmap, char *id, enum types type, void *sub);

#endif
