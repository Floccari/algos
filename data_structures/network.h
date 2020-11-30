#ifndef NETWORK_H
#define NETWORK_H

#include "list.h"
#include "hashmap.h"

enum dfs_colors {
    WHITE,
    GRAY,
    BLACK
};

enum tr_lists {
    TR,
    TR_IN,
    TR_OUT
};

struct state {
    char *id;
    enum dfs_colors color;
    char *delta;
    bool final;
    bool exit;
    // 2 bytes
    void *value;    // used to store various pointers
    struct list *tr_in;
    struct list *tr_out;
};

struct action {
    char *event;
    struct link *link;
};

enum label_types {
    OBSERVABILITY,
    RELEVANCE
};

struct label {
    char *id;
    enum label_types type;
};

struct transition {
    char *id;
    void *value;    // used to store various pointers
    struct action *act_in;
    struct list *act_out;
    struct label *obs;
    struct label *rel;
    struct state *src;
    struct state *dest;
};

struct automaton {
    char *id;
    struct list *states;
    struct state *initial;
    struct list *transitions;
    struct map_item **sttr_hashmap;
    /*struct list *lk_in;
      struct list *lk_out;*/
};

struct link {
    char *id;
    int index;    // used by bspace_compute and comp_compute (../features/bspace.h)
    // 4 bytes
    struct automaton *src;
    struct automaton *dest;
};

struct context {
    char *id;
    int aut_amount;
    int lk_amount;
    struct state **states;    // current state of every automaton
    char **buffers;    // link buffers
    struct list *current_obs;    // list item containing current obs label
    int obs_index;    // amount of labels processed so far
    // 4 bytes
};

struct network {
    char *id;
    int aut_amount;
    int lk_amount;
    struct list *automatons;
    struct list *events;
    struct list *links;
    struct list *observation;    // stored in REVERSE ORDER
};

struct state *state_create(char *id);

void state_attach(struct automaton *aut, struct state *st);

void state_detach(struct automaton *aut, struct state *st);

struct action *action_create();

struct label *label_create(char *id, enum label_types type);

struct label *label_copy(struct label *lab);

void label_destroy(struct label *lab);

struct transition *transition_create(char *id);

void transition_destroy(struct transition *tr);

void transition_attach(struct automaton *aut, struct transition *tr);

void transition_detach(struct automaton *aut, struct transition *tr);

struct automaton *automaton_create(char *id);

struct link *link_create(char *id);

struct context *context_create(int aut_amount, int lk_amount);

char *context_id_create(struct context *c);

struct context *context_copy(struct context *c);

void context_destroy(struct context *c);

struct network *network_create(char *id);

void network_serialize(FILE *fc, struct network *net);

void network_print_subs(FILE *fc, struct network *net, struct network *comp_net, bool comp);

void network_to_dot(FILE *fc, struct network *net);

char *state_id_create(long index);

char *transition_id_create(long index);

char *univ_tr_id_create(long index);

struct label *label_cat_create(struct label *lab1, struct label *lab2);

struct label *label_alt_create(struct label *lab1, struct label *lab2);

struct label *label_cat_auto_create(struct label *lab1, struct label *lab_auto, struct label *lab2);

#endif
