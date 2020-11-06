#include "network.h"

struct state *state_create(char *id) {
    struct state *st = malloc(sizeof (struct state));
    memset(st, 0, sizeof (struct state));
    st->id = id;
    st->color = WHITE;    // for pruning with DFS
    
    return st;
}

void state_destroy(struct state *st) {
    free(st->id);

    if (st->context)
	free(st->context);

    // state_detach already cleared tr_in and tr_out
    
    free(st);
}

void state_detach(struct automaton *aut, struct state *st) {
    struct list *ls = st->tr_in;
    
    /*** remove all incoming transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;

	transition_detach(aut, tr);
	transition_destroy(tr);

	ls = ls->next;
    }

    ls = st->tr_out;

    /*** remove all outgoing transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;

	transition_detach(aut, tr);
	transition_destroy(tr);

	ls = ls->next;
    }

    /*** remove state from the automaton ***/
    aut->states = search_and_remove(aut->states, st);
}

struct action *action_create() {
    struct action *act = malloc(sizeof (struct action));
    memset(act, 0, sizeof (struct action));
    
    return act;
}

struct label *label_create(char *id, enum label_types type) {
    struct label *l = malloc(sizeof (struct label));
    l->id = id;
    l->type = type;

    return l;
}

struct transition *transition_create(char *id) {
    struct transition *tr = malloc(sizeof (struct transition));
    memset(tr, 0, sizeof (struct transition));
    tr->id = id;

    return tr;
}

void transition_destroy(struct transition *tr) {
    free(tr->id);
    free(tr->act_in);

    while (tr->act_out) {
	free(tr->act_out->value);
	tr->act_out = item_remove(tr->act_out, tr->act_out);
    }

    free(tr);
}

void transition_attach(struct automaton *aut, struct transition *tr) {
    struct state *st = tr->src;

    st->tr_out = head_insert(st->tr_out,
			     list_create(tr));

    st = tr->dest;

    st->tr_in = head_insert(st->tr_in,
			    list_create(tr));

    /*** add to the automaton ***/
    aut->transitions = head_insert(aut->transitions,
				   list_create(tr));
}

void transition_detach(struct automaton *aut, struct transition *tr) {
    struct state *st = tr->src;

    st->tr_out = search_and_remove(st->tr_out, tr);

    st = tr->dest;

    st->tr_in = search_and_remove(st->tr_in, tr);

    /*** remove from the automaton ***/
    aut->transitions = search_and_remove(aut->transitions, tr);
}

struct automaton *automaton_create(char *id) {
    struct automaton *aut = malloc(sizeof (struct automaton));
    memset(aut, 0, sizeof (struct automaton));
    aut->id = id;

    return aut;
}

struct link *link_create(char *id) {
    struct link *ln = malloc(sizeof (struct link));
    memset(ln, 0, sizeof (struct link));
    ln->id = id;

    return ln;
}

struct context *context_create(int aut_amount, int lk_amount) {
    struct context *c = malloc(sizeof (struct context));
    c->states = calloc(aut_amount, sizeof (struct state *));
    c->buffers = calloc(lk_amount, sizeof (char *));

    c->aut_amount = aut_amount;
    c->lk_amount = lk_amount;

    return c;
}

struct context *context_copy(struct context *c) {
    struct context *copy = context_create(c->aut_amount, c->lk_amount);
    memcpy(copy->states, c->states, sizeof (struct state *) * c->aut_amount);
    memcpy(copy->buffers, c->buffers, sizeof (char *) * c->lk_amount);
    copy->current_obs = c->current_obs;
    copy->obs_index = c->obs_index;

    return copy;
}

void context_destroy(struct context *c) {
    free(c->states);
    free(c->buffers);
    free(c);
}

char *context_digest(struct context *c) {
    char *digest = calloc(sizeof (char), c->aut_amount + c->lk_amount + 1);
    digest[c->aut_amount + c->lk_amount] = '\0';

    for (int i = 0; i < c->aut_amount; i++)
	digest[i] = hash(c->states[i]->id, 89) + 33;    // characters from 33 to 122
	
    for (int i = 0; i < c->lk_amount; i++)
	if (c->buffers[i])
	    digest[i + c->aut_amount] = hash(c->buffers[i], 89) + 33;    // characters from 33 to 122
	else
	    digest[i + c->aut_amount] = 123;    // link is empty, use character 123

    return digest;
}

bool context_compare(struct context *c1, struct context *c2) {
    if (memcmp(c1->states, c2->states, sizeof (struct state *) * c1->aut_amount) == 0 &&
	memcmp(c1->buffers, c2->buffers, sizeof (char *) * c1->lk_amount) == 0 &&
	c1->obs_index == c2->obs_index)
	return true;

    return false;
}

struct map_item *context_search(struct map_item **hashmap, struct context *c) {
    char *id = context_digest(c);
    int key = hash(id, HASH_TABLE_SIZE);
    struct map_item *item = hashmap[key];
    
    while (item)
	if (item->type == CONTEXT && strcmp(item->id, id) == 0 && context_compare((struct context *) item->value, c))
	    return item;
	else
	    item = item->next;

    return item;
}

struct network *network_create(char *id) {
    struct network *net = malloc(sizeof (struct network));
    memset(net, 0, sizeof (struct network));
    net->id = id;

    return net;
}

void network_serialize(FILE *fc, struct network *net) {
    /*** network ***/
    fprintf(fc, "network %s:\n", net->id);
    fprintf(fc, "\tautomatons: ");

    /*** net automatons ***/
    struct list *l = get_last(net->automatons);

    if (l) {
	struct automaton *aut = (struct automaton *) l->value;
	fprintf(fc, "%s", aut->id);

	l = l->prev;
    
	while (l) {
	    struct automaton *aut = (struct automaton *) l->value;
	    fprintf(fc, ", %s", aut->id);
	    
	    l = l->prev;
	}
	
	fprintf(fc, ";\n");
    }

    /*** net events ***/
    l = get_last(net->events);

    if (l) {
	char *e = (char *) l->value;
	fprintf(fc, "\tevents: %s", e);

	l = l->prev;

	while (l) {
	    char *e = (char *) l->value;
	    fprintf(fc, ", %s", e);
	    
	    l = l->prev;
	}

	fprintf(fc, ";\n");
    }

    /*** net links ***/
    l = get_last(net->links);

    if (l)
	fprintf(fc, "\n");

    while (l) {
	struct link *lk = (struct link *) l->value;
	fprintf(fc, "\t%s %s -> %s;\n", lk->id, lk->src->id, lk->dest->id);

	l = l->prev;
    }

    fprintf(fc, "end\n");

    /*** automatons ***/
    l = get_last(net->automatons);

    while(l) {
	struct automaton *aut = (struct automaton *) l->value;
	fprintf(fc, "\n");
	fprintf(fc, "automaton %s:\n", aut->id);
	fprintf(fc, "\tstates: ");

	/*** aut states and inital ***/
	struct list *ls = get_last(aut->states);

	if (ls) {
	    struct state *s = (struct state *) ls->value;

	    if (s->final)
		fprintf(fc, "(%s)", s->id);
	    else
		fprintf(fc, "%s", s->id);		

	    ls = ls->prev;
    
	    while (ls) {
		struct state *s = (struct state *) ls->value;

		if (s->final)
		    fprintf(fc, ", (%s)", s->id);
		else
		    fprintf(fc, ", %s", s->id);		
		
		ls = ls->prev;
	    }
	    
	    fprintf(fc, ";\n");
	}

	fprintf(fc, "\tinitial: %s;\n", aut->initial->id);

	/*** aut transitions ***/
	ls = get_last(aut->transitions);

	if (ls)
	    fprintf(fc, "\n");

	while (ls) {
	    struct transition *tr = (struct transition *) ls->value;
	    fprintf(fc, "\t%s %s -> %s", tr->id, tr->src->id, tr->dest->id);

	    if (tr->obs)
		fprintf(fc, " obs \"%s\"", tr->obs->id);

	    if (tr->rel)
		fprintf(fc, " rel \"%s\"", tr->rel->id);

	    if (tr->act_in)
		fprintf(fc, " in \"%s(%s)\"", tr->act_in->event, tr->act_in->link->id);

	    if (tr->act_out) {
		struct list *lt = get_last(tr->act_out);
		
		if (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, " out \"%s(%s)", act->event, act->link->id);
		    
		    lt = lt->prev;
		}
		
		while (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, ", %s(%s)", act->event, act->link->id);
		    
		    lt = lt->prev;
		}
		
		fprintf(fc, "\"");
	    }

	    fprintf(fc, ";\n");
	    
	    ls = ls->prev;
	}

	fprintf(fc, "end\n");

	l = l->prev;
    }

    /*** observation ***/
    l = get_last(net->observation);

    if (l) {
	struct label *lab = l->value;
	
	fprintf(fc, "\n");
	fprintf(fc, "obs: %s", lab->id);

	l = l->prev;
	
	while (l) {
	    lab = l->value;
	    
	    fprintf(fc, ", %s", lab->id);

	    l = l->prev;
	}

	fprintf(fc, ";\n");
    }
}

void network_print_subs(FILE *fc, struct network *net, struct network *comp_net, bool comp) {
    fprintf(fc, "# SUBSTITUTIONS\n");
    fprintf(fc, "#");

    /*** legend ***/
    struct list *l = get_last(net->automatons);

    while (l) {
	struct automaton *aut = (struct automaton *) l->value;
	
	fprintf(fc, "\t%s", aut->id);

	l = l->prev;
    }

    fprintf(fc, "\t|");

    l = get_last(net->links);

    while (l) {
	struct link *lk = (struct link *) l->value;

	fprintf(fc, "\t%s", lk->id);

	l = l->prev;
    }

    if (comp)
	fprintf(fc, "\t|\tindex");
    
    fprintf(fc, "\n#\n");

    /*** contexts ***/
    struct automaton *aut =  (struct automaton *) comp_net->automatons->value;
    l = get_last(aut->states);
    
    while (l) {
	struct state *st = (struct state *) l->value;

	fprintf(fc, "# %s: ", st->id);

	struct context *c = st->context;

	/*** states ***/
	for (int i = net->aut_amount - 1; i >= 0; i--)
	    fprintf(fc, "\t%s", c->states[i]->id);

	fprintf(fc, "\t");

	/*** links ***/
	for (int i = net->lk_amount - 1; i >= 0; i--)
	    fprintf(fc, "\t%s", c->buffers[i]);

	/*** index ***/
	if (comp)
	    fprintf(fc, "\t\t%d", c->obs_index);
	
	fprintf(fc, "\n");

	l = l->prev;
    }
    
    fprintf(fc, "\n");
}

void network_to_dot(FILE *fc, struct network *net) {
    fprintf(fc, "digraph %s {\n", net->id);
    fprintf(fc, "\tcompound = true;\n");
    fprintf(fc, "\n");

    struct list *l = get_last(net->automatons);

    /*** automatons ***/
    while (l) {
	struct automaton *aut = (struct automaton *) l->value;

	fprintf(fc, "\tsubgraph cluster_%s {\n", aut->id);
	fprintf(fc, "\t\tlabel = %s\n", aut->id);
	fprintf(fc, "\n");
	
	fprintf(fc, "\t\t%s_init£ [shape = point]\n", aut->id);

	struct list *lt = get_last(aut->states);

	/*** states ***/
	while (lt) {
	    struct state *st = (struct state *) lt->value;

	    if (st->final)
		fprintf(fc, "\t\t%s_%s [label = \"%s\", shape = doublecircle]\n", aut->id, st->id, st->id);
	    else
		fprintf(fc, "\t\t%s_%s [label = \"%s\"]\n", aut->id, st->id, st->id);

	    lt = lt->prev;
	}

	lt = get_last(aut->transitions);

	fprintf(fc, "\n");
	fprintf(fc, "\t\t%s_init£ -> %s_%s\n", aut->id, aut->id, aut->initial->id);

	/*** transitions ***/
	while (lt) {
	    struct transition *tr = (struct transition *) lt->value;

	    fprintf(fc, "\t\t%s_%s -> %s_%s\n", aut->id, tr->src->id, aut->id, tr->dest->id);

	    lt = lt->prev;
	}

	fprintf(fc, "\t}\n");

	l = l->prev;

	if (l)
	    fprintf(fc, "\n");
    }

    l = get_last(net->links);

    if (l)
	fprintf(fc, "\n");

    /*** links ***/
    while (l) {
	struct link *lk = (struct link *) l->value;

	fprintf(fc, "\t%s_%s -> %s_%s [ltail = cluster_%s, lhead = cluster_%s]\n",
		lk->src->id, lk->src->initial->id, lk->dest->id, lk->dest->initial->id,
		lk->src->id, lk->dest->id);

	l = l->prev;
    }

    fprintf(fc, "}\n");
}


char *state_id_create(int index) {
    char *id = calloc(sizeof (char), 6);    // should use log(index) instead
    sprintf(id, "%d", index);

    return id;
}

char *transition_id_create(int index) {
    char *id = calloc(sizeof (char), 7);    // should use log(index) instead
    sprintf(id, "t%d", index);

    return id;
}

