#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


struct list {
    void *value;
    struct list *prev;
    struct list *next;
};


struct list *list_create(void *value);

struct list *get_last(struct list *l);

struct list *head_insert(struct list *l, struct list *item);

struct list *item_remove(struct list *l, struct list *item);

struct list *search_and_remove(struct list *l, void *value);

#endif
