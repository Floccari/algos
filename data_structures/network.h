#ifndef NETWORK_H
#define NETWORK_H

#include "list.h"
#include "hashmap.h"

enum dfs_colors {
    WHITE,
    GRAY,
    BLACK
};

struct state {
    char *id;
    enum dfs_colors color;
    bool final;
    // 3 bytes
    struct context *context;    // assigned by obs_space (defined in func.h)
    struct list *tr_in;
    struct list *tr_out;
};

struct action {
    char *event;
    struct link *link;
};

struct transition {
    char *id;
    struct action *act_in;
    struct list *act_out;
    char *obs;
    char *rel;
    struct state *src;
    struct state *dest;
};

struct automaton {
    char *id;
    struct list *states;
    struct state *initial;
    struct list *transitions;
    /*struct list *lk_in;
      struct list *lk_out;*/
};

struct link {
    char *id;
    int index;    // used by comp_space (defined in func.h)
    // 4 bytes
    struct automaton *src;
    struct automaton *dest;
};

struct context {
    int aut_amount;
    int lk_amount;
    struct state **states;    // current state of every automaton
    char **buffers;    // link buffers
};

struct network {
    char *id;
    int aut_amount;
    int lk_amount;
    struct list *automatons;
    struct list *events;
    struct list *links;
};

struct state *state_create(char *id);

struct action *action_create();

struct transition *transition_create(char *id);

struct automaton *automaton_create(char *id);

struct link *link_create(char *id);

struct context *context_create(int aut_amount, int lk_amount);

struct context *context_copy(struct context *c);

void context_destroy(struct context *c);

char *context_digest(struct context *c);

bool context_compare(struct context *c1, struct context *c2);

struct map_item *context_search(struct map_item **hashmap, struct context *c);

struct network *network_create(char *id);

void network_serialize(FILE *fc, struct network *net);

void network_print_subs(FILE *fc, struct network *net, struct network *comp_net);

void network_to_dot(FILE *fc, struct network *net);

#endif
