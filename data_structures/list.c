#include "list.h"

struct list *list_create(void *value) {
    struct list *l = malloc(sizeof (struct list));
    memset(l, 0, sizeof(struct list));
    l->value = value;

    return l;
}

struct list *get_last(struct list *l) {
    if (l)
	while (l->next)
	    l = l->next;

    return l;
}

struct list *head_insert(struct list *l, struct list *node) {
    if (l) {
	l->prev = node;
	node->next = l;
    }

    return node;
}

struct list *item_remove(struct list *l, struct list *item) {
    if (item->next)
	item->next->prev = item->prev;
    
    if (item->prev) {
	item->prev->next = item->next;
	return l;
    } else
	return item->next;
}

/* struct list *search_and_remove(struct list *l, void *value) { */
/*     struct list *current = l; */
    
/*     while (current) { */
/* 	if (current->value == value) { */
/* 	    struct list *to_return = item_remove(l, current); */
/* 	    free(current); */
/* 	    return to_return;    // exits here */
/* 	} */

/* 	current = current->next; */
/*     } */

/*     // in case it was not found */
/*     return l; */
/* } */
