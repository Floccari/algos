#include "dctor.h"

struct automaton *s_aut;
struct list *visited;
long tr_amount;

struct automaton *get_silent_space(struct automaton *bspace_aut);
struct automaton *get_silent(struct state *st);
void silent_visit(struct state *st);


struct automaton *get_diagnosticator(struct automaton *bspace_aut) {
    /*** compute the silent closure space ***/
    struct automaton *sspace_aut = get_silent_space(bspace_aut);

    sspace_aut->id = "dctor";

    struct list *l = sspace_aut->states;

    /*** foreach sspace state (closure) ***/
    while (l) {
	struct state *st = (struct state *) l->value;
	struct automaton *closure = (struct automaton *) st->value;

	struct list *ls = closure->states;
	
	/*** mark exit silent states as finals ***/
	/*** bspace states remain unaltered ***/
	while (ls) {
	    struct state *s_st = (struct state *) ls->value;

	    if (s_st->exit)
		s_st->final = true;
		
	    ls = ls->next;
	}

	/*** compute the split regexp ***/
	struct list *exp = get_split_diag(closure);

	struct label *cl_regexp = NULL;
	bool initialized = false;

	/*** foreach expression ***/
	while (exp) {
	    struct transition *tr = (struct transition *) exp->value;

	    struct list *lt = st->tr_out;
	    
	    /*** foreach outgoing transition ***/
	    while (lt) {
		struct transition *ts = (struct transition *) lt->value;

		/*** if this transition was generated from this exit state ***/
		if (ts->value == tr->value) {

		    /*** concatenate regexp with ts->rel ***/
		    ts->rel = label_cat_create(tr->rel, ts->rel);
		}

		lt = lt->next;
	    }
	    
	    struct state *bs = (struct state *) tr->value;
	
	    /*** if this state is final (not exit) ***/
	    /*** we know because bs is a bspace state (unaltered) ***/
	    if (bs->final) {

		/*** set sspace state (closure) as final ***/
		st->final = true;
		
		/*** build the appropriate label ***/
		if (initialized) 
		    cl_regexp = label_alt_create(cl_regexp, tr->rel);
		else {
		    cl_regexp = tr->rel;
		    initialized = true;
		}
	    }

	    exp = exp->next;
	}

	/*** assign cl_regexp to st ***/
	if (cl_regexp)
	    st->delta = cl_regexp->id;
	
	l = l->next;
    }

    return sspace_aut;
}

char *diagnosticate(struct automaton *dctor, struct list *observation) {
    struct map_item **s_hashmap = hashmap_create();

    struct state *initial = dctor->initial;
    
    /*** insert state ***/
    hashmap_insert(s_hashmap,
		   map_item_create(initial->id, STATE, initial));

    struct list *l = get_last(observation);
    
    /*** foreach label in the observation ***/
    while (l) {
	struct label *lab = (struct label *) l->value;

	struct map_item **new_hashmap = hashmap_create();
	bool empty = true;
	
	/*** foreach state in s_hashmap ***/
	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
	    if (s_hashmap[i]) {
		struct map_item *item = s_hashmap[i];
		
		while (item) {
		    struct state *st = (struct state *) item->value;
		    struct list *lt = st->tr_out;

		    /*** foreach outgoing transition ***/
		    while (lt) {
			struct transition *tr = (struct transition *) lt->value;

			if (tr->obs->id == lab->id) {
			    empty = false;
			    
			    struct label *new_lab = label_cat_create((struct label *) st->value, tr->rel);
			    struct state *dest = tr->dest;

			    /*** search in new_hashmap ***/
			    struct map_item *s_item = hashmap_search(new_hashmap, dest->id, STATE);

			    if (s_item) {
				/*** update existing state ***/
				struct state *s = (struct state *) item->value;

				s->value = label_alt_create((struct label *) s->value, new_lab);
			    } else {
				dest->value = new_lab;
				/*** insert state ***/
				hashmap_insert(new_hashmap,
					       map_item_create(dest->id, STATE, dest));
			    }
			}

			lt = lt->next;
		    }
		    
		    item = item->next;
		}
	    }	
	}

	/*** s_hashmap <- new_hashmap ***/
	struct map_item **tmp = s_hashmap;
	s_hashmap = new_hashmap;
	
	if (!empty)
	    hashmap_empty(tmp, false);

	free(tmp);

	l = l->prev;
    }

    struct label *diagnosis = NULL;
    bool initialized = false;

    /*** foreach state in s_hashmap ***/
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
	if (s_hashmap[i]) {
	    struct map_item *item = s_hashmap[i];
	    
	    while (item) {
		struct state *st = item->value;
		struct label *lab = NULL;

		if (st->delta)
		    lab = label_create(st->delta, RELEVANCE);
		
		if (st->final) {
		    if (initialized)
			diagnosis = label_alt_create(diagnosis,
						     label_cat_create((struct label *) st->value, lab));
		    else {
			diagnosis = label_cat_create((struct label *) st->value, lab);
			initialized = true;
		    }
		}
			
		item = item->next;
	    }
	}
    }

    if (diagnosis)
	return diagnosis->id;
    else
	return NULL;
}


struct automaton *get_silent_space(struct automaton *bspace_aut) {
    struct automaton *sspace_aut = automaton_create("sspace");
    struct map_item **s_hashmap = hashmap_create();

    /*** some lists are cycled in reverse order to make the output automaton prettier ***/
    
    struct list *l = get_last(bspace_aut->states);

    /*** foreach bspace state ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	struct list *lt = st->tr_in;
	bool obs = false;
	bool initial = (st == bspace_aut->initial);

	/*** check if there is an observable incoming transition ***/
	while (lt) {
	    struct transition *tr = (struct transition *) lt->value;

	    if (tr->obs) {
		obs = true;
		break;
	    }
	    
	    lt = lt->next;
	}

	if (initial || obs) {
	    /*** compute the silent closure of the current state ***/
	    struct automaton *closure = get_silent(st);

	    /*** create a new state in the silent space ***/
	    struct state *s_st = state_create(st->id);

	    state_attach(sspace_aut, s_st);

	    if (initial)
		sspace_aut->initial = s_st;

	    /*** save pointer to the silent closure inside the state ***/
	    s_st->value = closure;
	}

	l = l->prev;
    }

    l = sspace_aut->states;

    /*** foreach sspace state (closure) ***/
    while (l) {
	struct state *st = (struct state *) l->value;
	struct automaton *closure = (struct automaton *) st->value;
	
	struct list *ls = closure->states;

	/*** foreach state in the closure ***/
	while (ls) {
	    struct state *s_st = (struct state *) ls->value;

	    /*** add st to the hashmap ***/
	    char *id = calloc(strlen(st->id) + strlen(s_st->id) + 2, sizeof (char));
	    strcpy(id, st->id);

	    char *p = id + strlen(st->id);
	    *p++ = '#';
	    *p++ = '\0';	
	    
	    strcat(id, s_st->id);

	    hashmap_insert(s_hashmap,
			   map_item_create(id, STATE, st));

	    ls = ls->next;
	}

	l = l->next;
    }

    struct list *lt = get_last(bspace_aut->transitions);
    tr_amount = 0;

    /*** foreach observable bspace transition ***/
    while (lt) {
	struct transition *tr = lt->value;

	if (tr->obs) {
	    /*** build dest lookup id ***/
	    char *id = calloc((2 * strlen(tr->dest->id)) + 2, sizeof (char));
	    strcpy(id, tr->dest->id);

	    char *p = id + strlen(tr->dest->id);
	    *p++ = '#';
	    *p++ = '\0';
	    
	    strcat(id, tr->dest->id);

	    struct map_item *item = hashmap_search(s_hashmap, id, STATE);
	    struct state *dest = (struct state *) item->value;

	    /*** cleanup id ***/
	    free(id);

	    struct list *l = get_last(sspace_aut->states);

	    /*** foreach sspace state (closure) ***/
	    while (l) {
		struct state *st = (struct state *) l->value;

		/*** build src lookup id ***/
		char *id = calloc(strlen(st->id) + strlen(tr->src->id) + 2, sizeof (char));
		strcpy(id, st->id);

		char *p = id + strlen(st->id);
		*p++ = '#';
		*p++ = '\0';
		
		strcat(id, tr->src->id);

		item = hashmap_search(s_hashmap, id, STATE);

		if (item) {
		    struct state *src = (struct state *) item->value;
		    
		    /*** create the transition and attach it ***/
		    struct transition *s_tr = transition_create(transition_id_create(tr_amount++));
		    s_tr->src = src;
		    s_tr->dest = dest;
		    
		    s_tr->obs = tr->obs;
		    s_tr->rel = tr->rel;
		    s_tr->value = tr->src;
		    
		    transition_attach(sspace_aut, s_tr);
		}
		    
		/*** cleanup id ***/
		free(id);

		l = l->prev;
	    }
	}

	lt = lt->prev;
    }

    /*** cleanup ***/
    hashmap_empty(s_hashmap, true);
    free(s_hashmap);
    
    return sspace_aut;
}

struct automaton *get_silent(struct state *st) {
    s_aut = automaton_create("silent");
    visited = NULL;
    tr_amount = 0;

    /*** create new silent state and insert it ***/
    struct state *s_st = state_create(st->id);

    state_attach(s_aut, s_st);
    s_aut->initial = s_st;

    /*** save pointer to silent state into current state ***/
    st->value = s_st;

    /*** and the opposite too ***/
    s_st->value = st;    

    /*** recursive step ***/
    silent_visit(st);

    /*** reset colors ***/
    struct list *l = visited;

    while (l) {
	struct state *s = (struct state *) l->value;
	s->color = WHITE;

	struct list *next = l->next;
	free(l);
	l = next;
    }

    return s_aut;
}

void silent_visit(struct state *st) {
    st->color = GRAY;

    /*** add state to visited list (to reset colors later) ***/
    visited = head_insert(visited,
			  list_create(st));

    struct list *l = st->tr_out;

    while (l) {
	struct transition *tr = (struct transition *) l->value;

	if (!tr->obs) {
	    struct state *next = tr->dest;

	    if (next->color == WHITE) {
		/*** not visited yet ***/
		
		/*** create a new silent state and add it ***/
		struct state *s_next = state_create(next->id);

		state_attach(s_aut, s_next);
		s_next->final = next->final;

		/*** save pointer to silent state into next state ***/
		next->value = s_next;

		/*** and the opposite too ***/
		s_next->value = next;

		/*** connect st->silent and next->silent ***/
		struct transition *s_tr = transition_create(transition_id_create(tr_amount++));
		s_tr->src = (struct state *) st->value;
		s_tr->dest = (struct state *) next->value;
		s_tr->rel = tr->rel;

		transition_attach(s_aut, s_tr);
		
		/*** visit next state ***/
		silent_visit(next);
	    } else {
		/*** silent state already exists ***/

		/*** connect st->silent and next->silent ***/
		struct transition *s_tr = transition_create(transition_id_create(tr_amount++));
		s_tr->src = (struct state *) st->value;
		s_tr->dest = (struct state *) next->value;
		s_tr->rel = tr->rel;

		transition_attach(s_aut, s_tr);
	    }
	} else {
	    /*** mark as exit state ***/
	    struct state *s_st = (struct state *) st->value;
	    s_st->exit = true;
	}
	
	
	l = l->next;
    }

    st->color = BLACK;    // not really needed for now    
}
