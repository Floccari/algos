#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


struct list_item {
    void *value;
    struct list_item *prev;
    struct list_item *next;
};

struct list {
    struct list_item *head;
    struct list_item *tail;
    size_t nelem;
};


struct list_item *list_item_create(void *value);

struct list *list_create();

void head_insert(struct list *l, struct list_item *item);

void tail_insert(struct list *l, struct list_item *item);

void item_remove(struct list *l, struct list_item *item);

#endif
