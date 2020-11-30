#include "network.h"

struct state *state_create(char *id) {
    struct state *st = malloc(sizeof (struct state));
    memset(st, 0, sizeof (struct state));
    st->id = id;
    st->color = WHITE;    // for pruning with DFS
    
    return st;
}

void state_attach(struct automaton *aut, struct state *st) {
    struct list *l = list_create(st);

    aut->states = head_insert(aut->states, l);
    hashmap_insert(aut->sttr_hashmap,
		   map_item_create(st->id, STATE, l));
}

void state_detach(struct automaton *aut, struct state *st) {
    struct list *ls = st->tr_in;
    
    /*** remove all incoming transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;
	ls = ls->next;

	transition_detach(aut, tr);
	transition_destroy(tr);
    }

    ls = st->tr_out;

    /*** remove all outgoing transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;
	ls = ls->next;
	
	transition_detach(aut, tr);
	transition_destroy(tr);
    }

    /*** remove state from the automaton ***/
    struct map_item *item = hashmap_search_and_remove(aut->sttr_hashmap, st->id, STATE);
    struct list *list_item = (struct list *) item->value;
    
    aut->states = item_remove(aut->states, list_item);
    free(list_item);
    free(item);
}

struct action *action_create() {
    struct action *act = malloc(sizeof (struct action));
    memset(act, 0, sizeof (struct action));
    
    return act;
}

struct label *label_create(char *id, enum label_types type) {
    struct label *l = malloc(sizeof (struct label));
    // memset(l, 0, sizeof (struct label));
    l->id = id;
    l->type = type;

    return l;
}

struct label *label_copy(struct label *lab) {
    if (!lab)
	return NULL;
    
    struct label *new = label_create(NULL, lab->type);
    new->id = malloc(strlen(lab->id) + 1);
    strcpy(new->id, lab->id);
    new->type = lab->type;

    return new;
}

void label_destroy(struct label *lab) {
    free(lab->id);
    free(lab);
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

    if (tr->obs)
	label_destroy(tr->obs);

    if (tr->rel)
	label_destroy(tr->rel);

    free(tr);
}

void transition_attach(struct automaton *aut, struct transition *tr) {
    struct list *l = list_create(tr);
    struct state *st = tr->src;

    st->tr_out = head_insert(st->tr_out, l);
    hashmap_insert(aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR_OUT));    

    l = list_create(tr);
    st = tr->dest;

    st->tr_in = head_insert(st->tr_in, l);
    hashmap_insert(aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR_IN));    

    /*** add to the automaton ***/
    l = list_create(tr);
    aut->transitions = head_insert(aut->transitions, l);
    hashmap_insert(aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR));
}

void transition_detach(struct automaton *aut, struct transition *tr) {
    struct map_item *item = hashmap_search_with_sub_and_remove(aut->sttr_hashmap, tr->id,
								TRANSITION, (void *) TR_OUT);
    struct list *l = (struct list *) item->value;
    struct state *st = tr->src;

    st->tr_out = item_remove(st->tr_out, l);
    free(l);
    free(item);

    item = hashmap_search_with_sub_and_remove(aut->sttr_hashmap, tr->id,
					       TRANSITION, (void *) TR_IN);
    l = (struct list *) item->value;
    st = tr->dest;

    st->tr_in = item_remove(st->tr_in, l);
    free(l);
    free(item);

    /*** remove from the automaton ***/
    item = hashmap_search_with_sub_and_remove(aut->sttr_hashmap, tr->id,
					      TRANSITION, (void *) TR);
    l = (struct list *) item->value;
    
    aut->transitions = item_remove(aut->transitions, l);
    free(l);
    free(item);
}

struct automaton *automaton_create(char *id) {
    struct automaton *aut = malloc(sizeof (struct automaton));
    memset(aut, 0, sizeof (struct automaton));
    aut->id = id;
    aut->sttr_hashmap = hashmap_create();

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
    memset(c, 0, sizeof (struct context));
    
    c->states = calloc(aut_amount, sizeof (struct state *));
    c->buffers = calloc(lk_amount, sizeof (char *));

    c->aut_amount = aut_amount;
    c->lk_amount = lk_amount;

    return c;
}

char *context_id_create(struct context *c) {
    int size = 0;
    int slen[c->aut_amount];
    int elen[c->lk_amount];

    for (int i = 0; i < c->aut_amount; i++) {
	slen[i] = strlen(c->states[i]->id);
	size += slen[i] + 1;
    }

    for (int i = 0; i < c->lk_amount; i++) {
	elen[i] = c->buffers[i] ? strlen(c->buffers[i]) : 0;
	size += elen[i] + 1;
    }

    int obs_size = c->obs_index < 10 ? 1 :
	c->obs_index < 100 ? 2 :
	c->obs_index < 1000 ? 3 : 4;

    size += obs_size + 1;
    
    char *id = calloc(size, sizeof (char));
    char *p = id;
    
    for (int i = 0; i < c->aut_amount; i++) {
	strcpy(p, c->states[i]->id);
	p += slen[i];
	*p++ = '#';
    }

    for (int i = 0; i < c->lk_amount; i++){
	if (c->buffers[i]) {
	    strcpy(p, c->buffers[i]);
	    p += elen[i];
	}

	*p++ = '#';
    }

    sprintf(p, "%d", c->obs_index);
    p += obs_size;
    *p++ = '\0';
    
    return id;
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
    if (c->id)
	free(c->id);
    
    free(c->states);
    free(c->buffers);
    free(c);
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
		fprintf(fc, "[%s]", s->id);
	    else
		fprintf(fc, "%s", s->id);

	    if (s->delta)
		fprintf(fc, " \"%s\"", s->delta);

	    ls = ls->prev;
    
	    while (ls) {
		struct state *s = (struct state *) ls->value;
		
		if (s->final)
		    fprintf(fc, ", [%s]", s->id);
		else
		    fprintf(fc, ", %s", s->id);		

		if (s->delta)
		    fprintf(fc, " \"%s\"", s->delta);
		
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
		fprintf(fc, " in \"%s[%s]\"", tr->act_in->event, tr->act_in->link->id);

	    if (tr->act_out) {
		struct list *lt = get_last(tr->act_out);
		
		if (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, " out \"%s[%s]", act->event, act->link->id);
		    
		    lt = lt->prev;
		}
		
		while (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, ", %s[%s]", act->event, act->link->id);
		    
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

	struct context *c = (struct context *) st->value;

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


char *state_id_create(long index) {
    char *id = calloc(10, sizeof (char));    // should use log(index) instead
    sprintf(id, "%ld", index);

    return id;
}

char *transition_id_create(long index) {
    char *id = calloc(11, sizeof (char));    // should use log(index) instead
    sprintf(id, "t%ld", index);

    return id;
}

// internal, unique, not serializable
char *univ_tr_id_create(long index) {
    char *id = calloc(12, sizeof (char));    // should use log(index) instead
    sprintf(id, "_t%ld", index);

    return id;
}

struct label *label_cat_create(struct label *lab1, struct label *lab2) {
    int l1 = (lab1) ? strlen(lab1->id) : 0;
    int l2 = (lab2) ? strlen(lab2->id) : 0;

    if (l1 + l2 > 0) {
	char *id = calloc(l1 + l2 + 1, sizeof (char));
	char *p = id;

	if (lab1)
	    strcpy(p, lab1->id);

	p += l1;

	if (lab2)
	    strcpy(p, lab2->id);

	p += l2;
	*p++ = '\0';

	return label_create(id, RELEVANCE);
    }

    return NULL;
}

struct label *label_alt_create(struct label *lab1, struct label *lab2) {
    /*** reduction ***/
    if (lab1 && lab2) {
	int cmp = strcmp(lab1->id, lab2->id);
			 
	if (cmp == 0)
	    return label_copy(lab1);
	else if (cmp > 0) {
	    struct label *tmp = lab1;
	    lab1 = lab2;
	    lab2 = tmp;
	}
    }

    int l1 = (lab1) ? strlen(lab1->id) : 0;
    int l2 = (lab2) ? strlen(lab2->id) : 0;

    int extra = (lab1 && lab2) ? 1 : 3;

    /*** reduction ***/
    if (lab1 && lab2) {
	bool equal = true;

	for (int i = 0; i < l1 && i < l2; i++)
	    if (lab1->id[i] != lab2->id[i]) {
		equal = false;
		break;
	    }

	if (equal) {
	    if (l1 < l2 && lab2->id[l1] == '|')
		return label_copy(lab2);
	    else if (l2 < l1 && lab1->id[l2] == '|')
		return label_copy(lab1);
	}

	equal = true;
	
	for (int i = 0; i < l1 && i < l2; i++)
	    if (lab1->id[l1 - i - 1] != lab2->id[l2 - i - 1]) {
		equal = false;
		break;
	    }

	if (equal) {
	    if (l1 < l2 && lab2->id[l2 - l1 - 1] == '|')
		return label_copy(lab2);
	    else if (l2 < l1 && lab1->id[l1 - l2 - 1] == '|')
		return label_copy(lab1);
	}
    }

    /*** reduction ***/
    if (lab1 && !lab2) {
	if (lab1->id[0] == '(') {
	    int count = 1;
	    char *q = &lab1->id[1];

	    while (*q != '\0') {
		switch (*q) {
		case '(':
		    count++;
		    break;
		case ')':
		    count--;
		    break;
		default:
		    break;
		}

		if (count == 0)
		    break;

		q++;
	    }

	    if (*q++ == ')' && *q++ == '?' && *q++ == '\0')
		return label_copy(lab1);
	}
    }

    /*** reduction ***/
    if (lab2 && !lab1) {
	if (lab2->id[0] == '(') {
	    int count = 1;
	    char *q = &lab2->id[1];

	    while (*q != '\0') {
		switch (*q) {
		case '(':
		    count++;
		    break;
		case ')':
		    count--;
		    break;
		default:
		    break;
		}

		if (count == 0)
		    break;

		q++;
	    }

	    if (*q++ == ')' && *q++ == '?' && *q++ == '\0')
		return label_copy(lab2);
	}
    }
    
    if (l1 + l2 > 0) {
	char *id = calloc(l1 + l2 + extra + 1, sizeof (char));
	char *p = id;

	if (lab1) {
	    if (lab2) {
		strcpy(p, lab1->id);
		p += l1;
	    } else {
		*p++ = '(';
		strcpy(p, lab1->id);

		p += l1;
		*p++ = ')';
		*p++ = '?';
	    }
	}

	if (lab1 && lab2)
	    *p++ = '|';

	if (lab2) {
	    if (lab1) {
		strcpy(p, lab2->id);
		p += l2;			
	    } else {
		*p++ = '(';
		strcpy(p, lab2->id);
			    
		p += l2;
		*p++ = ')';
		*p++ = '?';
	    }
	}

	*p++ = '\0';

	return label_create(id, RELEVANCE);
    }

    return NULL;
}    

struct label *label_cat_auto_create(struct label *lab1, struct label *lab_auto, struct label *lab2) {
    int l1 = (lab1) ? strlen(lab1->id) : 0;
    int l2 = (lab2) ? strlen(lab2->id) : 0;
    int l_auto = (lab_auto) ? strlen(lab_auto->id) : 0;

    int extra = (lab_auto) ? 3 : 0;

    if (l1 + l2 + l_auto > 0) {
	char *id = calloc(l1 + l2 + l_auto + extra + 1, sizeof (char));
	char *p = id;
			
	if (lab1)
	    strcpy(p, lab1->id);
			
	p += l1;
			
	if (lab_auto) {
	    *p++ = '(';
	    strcpy(p, lab_auto->id);
			    
	    p += l_auto;
	    *p++ = ')';
	    *p++ = '*';
	}
			
	if (lab2)
	    strcpy(p, lab2->id);
			
	p += l2;
	*p++ = '\0';

	return label_create(id, RELEVANCE);
    }

    return NULL;
}
