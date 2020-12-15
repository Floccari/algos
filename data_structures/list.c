#include "list.h"

struct list_item *list_item_create(void *value) {
    struct list_item *item = malloc(sizeof (struct list_item));
    memset(item, 0, sizeof (struct list_item));
    item->value = value;

    return item;
}

struct list *list_create() {
    struct list *l = malloc(sizeof (struct list));
    memset(l, 0, sizeof (struct list));

    return l;
}

void head_insert(struct list *l, struct list_item *item) {
    if (l->head) {
	l->head->prev = item;
	item->next = l->head;

	l->head = item;
    } else {
	l->head = item;
	l->tail = item;
    }

    l->nelem++;
}

void tail_insert(struct list *l, struct list_item *item) {
    if (l->tail) {
	l->tail->next = item;
	item->prev = l->tail;

	l->tail = item;
    } else {
	l->head = item;
	l->tail = item;
    }

    l->nelem++;
}

void item_remove(struct list *l, struct list_item *item) {
    if (item->next)
	item->next->prev = item->prev;
    else
	l->tail = item->prev;

    if (item->prev)
	item->prev->next = item->next;
    else
	l->head = item->next;

    l->nelem--;
}
