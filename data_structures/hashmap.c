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

int hash(char *id, int m) {
    int h = 0;
    char *p = id;
    
    while (*p != '\0')
	h = ((h * 256) + *p++) % m;

    return  h;
}

void hashmap_insert(struct map_item **hashmap, struct map_item *item) {
    int key = hash(item->id, HASH_TABLE_SIZE);

    if (hashmap[key]) {
	struct map_item *last = hashmap[key];
	
	while (last->next)
	    last = last->next;

	last->next = item;
    } else
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

    return item;
}

struct map_item *hashmap_search_with_sub(struct map_item **hashmap, char *id, enum types type, void *sub) {
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];
    
    while (item)
	if (item->type == type && strcmp(item->id, id) == 0 && item->subvalue == sub)
	    return item;
	else
	    item = item->next;

    return item;
}


