#include "network.h"

struct state *state_create(char *id) {
    struct state *st = malloc(sizeof (struct state));
    memset(st, 0, sizeof (struct state));
    st->id = id;
    st->color = WHITE;    // for pruning with DFS

    return st;
}

void state_attach(struct automaton *aut, struct state *st) {
    struct list_item *l = list_item_create(st);

    tail_insert(&aut->states, l);
    hashmap_insert(&aut->sttr_hashmap,
		   map_item_create(st->id, STATE, l));
}

void state_detach(struct automaton *aut, struct state *st) {
    struct list_item *ls = st->tr_in.head;
    
    /*** remove all incoming transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;
	ls = ls->next;

	transition_detach(aut, tr);
	transition_destroy(tr);
    }

    ls = st->tr_out.head;

    /*** remove all outgoing transitions ***/
    while (ls) {
	struct transition *tr = (struct transition *) ls->value;
	ls = ls->next;
	
	transition_detach(aut, tr);
	transition_destroy(tr);
    }

    /*** remove state from the automaton ***/
    struct map_item *item = hashmap_search_and_remove(&aut->sttr_hashmap, st->id, STATE);
    ls = (struct list_item *) item->value;
    
    item_remove(&aut->states, ls);
    free(ls);
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

    while (tr->act_out.head) {
	struct list_item *l = tr->act_out.head;
	
	free(l->value);
	item_remove(&tr->act_out, l);
    }

    if (tr->obs)
	label_destroy(tr->obs);

    if (tr->rel)
	label_destroy(tr->rel);

    free(tr);
}

void transition_attach(struct automaton *aut, struct transition *tr) {
    struct list_item *l = list_item_create(tr);
    struct state *st = tr->src;

    tail_insert(&st->tr_out, l);
    hashmap_insert(&aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR_OUT));    

    l = list_item_create(tr);
    st = tr->dest;

    tail_insert(&st->tr_in, l);
    hashmap_insert(&aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR_IN));    

    /*** add to the automaton ***/
    l = list_item_create(tr);
    tail_insert(&aut->transitions, l);
    hashmap_insert(&aut->sttr_hashmap,
		   map_item_create_with_sub(tr->id, TRANSITION, l, (void *) TR));
}

void transition_detach(struct automaton *aut, struct transition *tr) {
    struct map_item *item = hashmap_search_with_sub_and_remove(&aut->sttr_hashmap, tr->id,
								TRANSITION, (void *) TR_OUT);
    struct list_item *l = (struct list_item *) item->value;
    struct state *st = tr->src;

    item_remove(&st->tr_out, l);
    free(l);
    free(item);

    item = hashmap_search_with_sub_and_remove(&aut->sttr_hashmap, tr->id,
					       TRANSITION, (void *) TR_IN);
    l = (struct list_item *) item->value;
    st = tr->dest;

    item_remove(&st->tr_in, l);
    free(l);
    free(item);

    /*** remove from the automaton ***/
    item = hashmap_search_with_sub_and_remove(&aut->sttr_hashmap, tr->id,
					      TRANSITION, (void *) TR);
    l = (struct list_item *) item->value;
    
    item_remove(&aut->transitions, l);
    free(l);
    free(item);
}

struct automaton *automaton_create(char *id, int hashmap_size) {
    struct automaton *aut = malloc(sizeof (struct automaton));
    memset(aut, 0, sizeof (struct automaton));
    aut->id = id;
    hashmap_buffer_allocate(&aut->sttr_hashmap, hashmap_size);

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
    struct list_item *l = net->automatons.head;

    if (l) {
	struct automaton *aut = (struct automaton *) l->value;
	fprintf(fc, "%s", aut->id);

	l = l->next;
    
	while (l) {
	    struct automaton *aut = (struct automaton *) l->value;
	    fprintf(fc, ", %s", aut->id);
	    
	    l = l->next;
	}
	
	fprintf(fc, ";\n");
    }

    /*** net events ***/
    l = net->events.head;

    if (l) {
	char *e = (char *) l->value;
	fprintf(fc, "\tevents: %s", e);

	l = l->next;

	while (l) {
	    char *e = (char *) l->value;
	    fprintf(fc, ", %s", e);
	    
	    l = l->next;
	}

	fprintf(fc, ";\n");
    }

    /*** net links ***/
    l = net->links.head;

    if (l)
	fprintf(fc, "\n");

    while (l) {
	struct link *lk = (struct link *) l->value;
	fprintf(fc, "\t%s %s -> %s;\n", lk->id, lk->src->id, lk->dest->id);

	l = l->next;
    }

    fprintf(fc, "end\n");

    /*** automatons ***/
    l = net->automatons.head;

    while(l) {
	struct automaton *aut = (struct automaton *) l->value;
	long st_amount = 0;
	long tr_amount = 0;

	fprintf(fc, "\n");
	fprintf(fc, "automaton %s:\n", aut->id);
	fprintf(fc, "\tstates: ");

	/*** aut states and inital ***/
	struct list_item *ls = aut->states.head;

	if (ls) {
	    struct state *s = (struct state *) ls->value;
	    st_amount++;

	    if (s->final)
		fprintf(fc, "[%s]", s->id);
	    else
		fprintf(fc, "%s", s->id);

	    if (s->delta)
		fprintf(fc, " \"%s\"", s->delta);

	    ls = ls->next;
    
	    while (ls) {
		struct state *s = (struct state *) ls->value;
		st_amount++;
		
		if (s->final)
		    fprintf(fc, ", [%s]", s->id);
		else
		    fprintf(fc, ", %s", s->id);		

		if (s->delta)
		    fprintf(fc, " \"%s\"", s->delta);
		
		ls = ls->next;
	    }
	    
	    fprintf(fc, ";\n");
	}

	fprintf(fc, "\tinitial: %s;\n", aut->initial->id);

	/*** aut transitions ***/
	ls = aut->transitions.head;

	if (ls)
	    fprintf(fc, "\n");

	while (ls) {
	    struct transition *tr = (struct transition *) ls->value;
	    tr_amount++;
	    
	    fprintf(fc, "\t%s %s -> %s", tr->id, tr->src->id, tr->dest->id);

	    if (tr->obs)
		fprintf(fc, " obs \"%s\"", tr->obs->id);

	    if (tr->rel)
		fprintf(fc, " rel \"%s\"", tr->rel->id);

	    if (tr->act_in)
		fprintf(fc, " in \"%s[%s]\"", tr->act_in->event, tr->act_in->link->id);

	    if (tr->act_out.head) {
		struct list_item *lt = tr->act_out.head;
		
		if (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, " out \"%s[%s]", act->event, act->link->id);
		    
		    lt = lt->next;
		}
		
		while (lt) {
		    struct action *act = (struct action *) lt->value;
		    
		    fprintf(fc, ", %s[%s]", act->event, act->link->id);
		    
		    lt = lt->next;
		}
		
		fprintf(fc, "\"");
	    }

	    fprintf(fc, ";\n");
	    
	    ls = ls->next;
	}

	fprintf(fc, "end\t# states: %li, transitions: %li\n", st_amount, tr_amount);

	l = l->next;
    }

    /*** observation ***/
    l = net->observation.head;

    if (l) {
	struct label *lab = l->value;
	
	fprintf(fc, "\n");
	fprintf(fc, "obs: %s", lab->id);

	l = l->next;
	
	while (l) {
	    lab = l->value;
	    
	    fprintf(fc, ", %s", lab->id);

	    l = l->next;
	}

	fprintf(fc, ";\n");
    }
}

void network_print_subs(FILE *fc, struct network *net, struct network *comp_net, bool comp) {
    fprintf(fc, "# SUBSTITUTIONS\n");
    fprintf(fc, "#");

    /*** legend ***/
    struct list_item *l = net->automatons.head;
    fprintf(fc, "\t");

    while (l) {
	struct automaton *aut = (struct automaton *) l->value;
	
	fprintf(fc, "\t%s", aut->id);

	l = l->next;
    }

    fprintf(fc, "\t|");

    l = net->links.head;

    while (l) {
	struct link *lk = (struct link *) l->value;

	fprintf(fc, "\t%s", lk->id);

	l = l->next;
    }

    if (comp)
	fprintf(fc, "\t|\tindex");
    
    fprintf(fc, "\n#\n");

    /*** contexts ***/
    struct automaton *aut =  (struct automaton *) comp_net->automatons.head->value;
    l = aut->states.head;
    
    while (l) {
	struct state *st = (struct state *) l->value;

	fprintf(fc, "# %s:\t", st->id);

	struct context *c = (struct context *) st->value;

	/*** states ***/
	for (int i = 0; i < net->aut_amount; i++)
	    fprintf(fc, "\t%s", c->states[i]->id);

	fprintf(fc, "\t");

	/*** links ***/
	for (int i = 0; i < net->lk_amount; i++)
	    fprintf(fc, "\t%s", c->buffers[i]);

	/*** index ***/
	if (comp)
	    fprintf(fc, "\t\t%d", c->obs_index);
	
	fprintf(fc, "\n");

	l = l->next;
    }
    
    fprintf(fc, "\n");
}

void network_to_dot(FILE *fc, struct network *net) {
    fprintf(fc, "digraph %s {\n", net->id);
    fprintf(fc, "\tcompound = true;\n");
    fprintf(fc, "\n");

    struct list_item *l = net->automatons.head;

    /*** automatons ***/
    while (l) {
	struct automaton *aut = (struct automaton *) l->value;

	fprintf(fc, "\tsubgraph cluster_%s {\n", aut->id);
	fprintf(fc, "\t\tlabel = %s\n", aut->id);
	fprintf(fc, "\n");
	
	fprintf(fc, "\t\t%s_init£ [shape = point]\n", aut->id);

	struct list_item *lt = aut->states.head;

	/*** states ***/
	while (lt) {
	    struct state *st = (struct state *) lt->value;

	    if (st->final)
		fprintf(fc, "\t\t%s_%s [label = \"%s\", shape = doublecircle]\n", aut->id, st->id, st->id);
	    else
		fprintf(fc, "\t\t%s_%s [label = \"%s\"]\n", aut->id, st->id, st->id);

	    lt = lt->next;
	}

	lt = aut->transitions.head;

	fprintf(fc, "\n");
	fprintf(fc, "\t\t%s_init£ -> %s_%s\n", aut->id, aut->id, aut->initial->id);

	/*** transitions ***/
	while (lt) {
	    struct transition *tr = (struct transition *) lt->value;

	    fprintf(fc, "\t\t%s_%s -> %s_%s\n", aut->id, tr->src->id, aut->id, tr->dest->id);

	    lt = lt->next;
	}

	fprintf(fc, "\t}\n");

	l = l->next;

	if (l)
	    fprintf(fc, "\n");
    }

    l = net->links.head;

    if (l)
	fprintf(fc, "\n");

    /*** links ***/
    while (l) {
	struct link *lk = (struct link *) l->value;

	fprintf(fc, "\t%s_%s -> %s_%s [ltail = cluster_%s, lhead = cluster_%s]\n",
		lk->src->id, lk->src->initial->id, lk->dest->id, lk->dest->initial->id,
		lk->src->id, lk->dest->id);

	l = l->next;
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
    if (lab1) {
	if (!lab2)
	    return label_copy(lab1);
    } else {
	if (!lab2)
	    return NULL;
	else
	    return label_copy(lab2);
    }
    
    int l1 = strlen(lab1->id);
    int l2 = strlen(lab2->id);

    int extra1 = strchr(lab1->id, '|') ? 2 : 0;
    int extra2 = strchr(lab2->id, '|') ? 2 : 0;    

    char *id = calloc(l1 + l2 + extra1 + extra2 + 1, sizeof (char));
    char *p = id;

    if (extra1)
	*p++ = '(';

    strcpy(p, lab1->id);
    p += l1;

    if (extra1)
	*p++ = ')';

    if (extra2)
	*p++ = '(';

    strcpy(p, lab2->id);
    p += l2;

    if (extra2)
	*p++ = ')';
	
    *p++ = '\0';

    return label_create(id, RELEVANCE);
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
    if (!lab_auto)
	return label_cat_create(lab1, lab2);
    
    int l1 = (lab1) ? strlen(lab1->id) : 0;
    int l2 = (lab2) ? strlen(lab2->id) : 0;
    int l_auto = strlen(lab_auto->id);

    int extra = 3;
    int extra1 = (lab1 && strchr(lab1->id, '|')) ? 2 : 0;
    int extra2 = (lab2 && strchr(lab2->id, '|')) ? 2 : 0;    

    char *id = calloc(l1 + l2 + l_auto + extra + extra1 + extra2 + 1, sizeof (char));
    char *p = id;
    
    if (extra1)
	*p++ = '(';
    
    if (lab1)
	strcpy(p, lab1->id);
    
    p += l1;
    
    if (extra1)
	*p++ = ')';
    
    *p++ = '(';
    strcpy(p, lab_auto->id);
    
    p += l_auto;
    *p++ = ')';
    *p++ = '*';
    
    if (extra2)
	*p++ = '(';
    
    if (lab2)
	strcpy(p, lab2->id);
    
    p += l2;
    
    if (extra2)
	*p++ = ')';
    
    *p++ = '\0';
    
    return label_create(id, RELEVANCE);
}

size_t maximum_state_amount(struct network *net) {
    size_t max = 0;
    struct list_item *la = net->automatons.head;

    while (la) {
	struct automaton *aut = (struct automaton *) la->value;
	size_t tot = aut->states.nelem;

	if (tot > max)
	    max = tot;
	
	la = la->next;
    }

    return max;
}
