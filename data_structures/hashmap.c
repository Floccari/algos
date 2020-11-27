#include "hashmap.h"

struct map_item *map_item_create(char *id, enum types type, void *value) {
    struct map_item *item = malloc(sizeof (struct map_item));
    memset(item, 0, sizeof (struct map_item));
    item->id = id;
    item->type = type;
    item->value = value;

    return item;
}

struct map_item *map_item_create_with_sub(char *id, enum types type, void *value, void *subvalue) {
    struct map_item *item = map_item_create(id, type, value);
    item->subvalue = subvalue;

    return item;
}

struct map_item **hashmap_create() {
    struct map_item **hashmap = calloc(HASH_TABLE_SIZE, sizeof (struct map_item *));
    memset(hashmap, 0, sizeof (struct map_item *) * HASH_TABLE_SIZE);

    return hashmap;
}

void hashmap_empty(struct map_item **hashmap, bool free_ids) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
	if (hashmap[i]) {
	    struct map_item *item = hashmap[i];
	    hashmap[i] = NULL;
	    
	    while (item) {
		struct map_item *next = item->next;

		if (free_ids)
		    free(item->id);
		
		free(item);
		item = next;
	    }
	}
}

int hash(char *id, int m) {
    int h = 0;
    char *p = id;
    
    while (*p != '\0')
	h = ((h * 256) + *p++) % m;

    return  h;
}

void hashmap_insert(struct map_item **hashmap, struct map_item *item) {
    int key = hash(item->id, HASH_TABLE_SIZE);

    item->next = hashmap[key];
    hashmap[key] = item;
}

struct map_item *hashmap_search(struct map_item **hashmap, char *id, enum types type) {
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];
    
    while (item)
	if (item->type == type && strcmp(item->id, id) == 0)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_search_with_sub(struct map_item **hashmap, char *id, enum types type, void *sub) {
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];
    
    while (item)
	if (item->type == type && item->subvalue == sub && strcmp(item->id, id) == 0)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_search_and_remove(struct map_item **hashmap, char *id, enum types type) {
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];

    if (item && item->type == type && strcmp(item->id, id) == 0) {
	hashmap[key] = item->next;
	return item;
    }
    
    while (item->next) {
	struct map_item *next = item->next;
	
	if (next->type == type && strcmp(next->id, id) == 0) {
	    item->next = next->next;
	    return next;
	} else
	    item = next;
    }

    return NULL;
}

struct map_item *hashmap_search_with_sub_and_remove(struct map_item **hashmap, char *id, enum types type, void *sub) {
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];

    if (item && item->type == type && item->subvalue == sub && strcmp(item->id, id) == 0) {
	hashmap[key] = item->next;
	return item;
    }
    
    while (item->next) {
	struct map_item *next = item->next;

	if (next->type == type && next->subvalue == sub && strcmp(next->id, id) == 0) {
	    item->next = next->next;
	    return next;
	}
	else
	    item = next;
    }

    return NULL;
}
