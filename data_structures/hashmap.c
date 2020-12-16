#include "hashmap.h"

#include <math.h>

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

struct hashmap *hashmap_create(size_t nelem) {
    struct hashmap *hashmap = malloc(sizeof (struct hashmap));
//    memset(hashmap, 0, sizeof (struct hashmap));

    hashmap_buffer_allocate(hashmap, nelem);
    return hashmap;
}

void hashmap_buffer_allocate(struct hashmap *hashmap, size_t nelem) {
    hashmap->buffer = calloc(nelem, sizeof (struct map_item *));

    if (!hashmap->buffer) {
	fprintf(stderr, "memory allocation failed during hashmap creation, exiting...\n");
	exit(-1);
    }
    
    memset(hashmap->buffer, 0, nelem * sizeof (struct map_item *));
    hashmap->nelem = nelem;
}

void hashmap_buffer_deallocate(struct hashmap *hashmap) {
    free(hashmap->buffer);
    memset(hashmap, 0, sizeof (struct hashmap));
}

void hashmap_empty(struct hashmap *hashmap, bool free_ids) {
    for (size_t i = 0; i < hashmap->nelem; i++)
	if (hashmap->buffer[i]) {
	    struct map_item *item = hashmap->buffer[i];
	    hashmap->buffer[i] = NULL;
	    
	    while (item) {
		struct map_item *next = item->next;

		if (free_ids)
		    free(item->id);
		
		free(item);
		item = next;
	    }
	}
}

void hashmap_destroy(struct hashmap *hashmap) {
    free(hashmap->buffer);
    free(hashmap);
}

int hash(char *id, size_t m) {
    size_t h = 0;
    float a = 0.618034;    // (sqrt(5) - 1) / 2;
    char k;
    
    while ((k = *id++)) {
	float term, fract, integ;
	
	term = ((h << 6) + k) * a;
	fract = modff(term, &integ);
	
	h = fract * m;
    }

    return  h;
}

void hashmap_insert(struct hashmap *hashmap, struct map_item *item) {
    size_t key = hash(item->id, hashmap->nelem);

    item->next = hashmap->buffer[key];
    hashmap->buffer[key] = item;
}

/*** compare pointers ***/

struct map_item *hashmap_search(struct hashmap *hashmap, char *id, enum types type) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];
    
    while (item)
	if (item->type == type && item->id == id)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_search_with_sub(struct hashmap *hashmap, char *id, enum types type, void *sub) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];
    
    while (item)
	if (item->type == type && item->subvalue == sub && item->id == id)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_search_and_remove(struct hashmap *hashmap, char *id, enum types type) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];

    if (item) {
	if (item->type == type && item->id == id) {
	    hashmap->buffer[key] = item->next;
	    return item;
	}
	
	while (item->next) {
	    struct map_item *next = item->next;
	    
	    if (next->type == type && next->id == id) {
		item->next = next->next;
		return next;
	    } else
		item = next;
	}
    }

    return NULL;
}

struct map_item *hashmap_search_with_sub_and_remove(struct hashmap *hashmap, char *id, enum types type, void *sub) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];

    if (item) {
	if (item->type == type && item->subvalue == sub && item->id == id) {
	    hashmap->buffer[key] = item->next;
	    return item;
	}
	
	while (item->next) {
	    struct map_item *next = item->next;
	    
	    if (next->type == type && next->subvalue == sub && next->id == id) {
		item->next = next->next;
		return next;
	    }
	    else
		item = next;
	}
    }

    return NULL;
}

/*** compare strings ***/

struct map_item *hashmap_strcmp_search(struct hashmap *hashmap, char *id, enum types type) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];
    
    while (item)
	if (item->type == type && strcmp(item->id, id) == 0)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_strcmp_search_with_sub(struct hashmap *hashmap, char *id, enum types type, void *sub) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];
    
    while (item)
	if (item->type == type && item->subvalue == sub && strcmp(item->id, id) == 0)
	    return item;
	else
	    item = item->next;

    return NULL;
}

struct map_item *hashmap_strcmp_search_and_remove(struct hashmap *hashmap, char *id, enum types type) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];

    if (item) {
	if (item->type == type && strcmp(item->id, id) == 0) {
	    hashmap->buffer[key] = item->next;
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
    }

    return NULL;
}

struct map_item *hashmap_strcmp_search_with_sub_and_remove(struct hashmap *hashmap, char *id, enum types type, void *sub) {
    size_t key = hash(id, hashmap->nelem);
    struct map_item *item = hashmap->buffer[key];

    if (item) {
	if (item->type == type && item->subvalue == sub && strcmp(item->id, id) == 0) {
	    hashmap->buffer[key] = item->next;
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
    }

    return NULL;
}
