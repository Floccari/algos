#include "dctor.h"

struct automaton *s_aut;
struct list *visited;
int tr_amount;

void silent_visit(struct state *st);
struct automaton *get_silent(struct state *st);


struct automaton *get_silent_space(struct automaton *bspace_aut) {
    struct automaton *sspace_aut = automaton_create("sspace");
    struct map_item **s_hashmap = hashmap_create();

    struct list *l = bspace_aut->states;

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

	    sspace_aut->states = head_insert(sspace_aut->states,
					     list_create(s_st));

	    if (initial)
		sspace_aut->initial = s_st;

	    /*** save pointer to the silent closure inside the state ***/
	    s_st->value = closure;
	}

	l = l->next;
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

	    /*** add st to the hashmap with id = strcat(st->id, s_st->id) ***/
	    char *id = calloc(strlen(st->id) + strlen(s_st->id) + 1, sizeof (char));
	    strcpy(id, st->id);
	    strcat(id, s_st->id);

	    hashmap_insert(s_hashmap,
			   map_item_create(id, STATE, st));

	    ls = ls->next;
	}

	l = l->next;
    }

    struct list *lt = bspace_aut->transitions;
    tr_amount = 0;

    /*** foreach observable bspace transition ***/
    while (lt) {
	struct transition *tr = lt->value;

	if (tr->obs) {
	    /*** build dest lookup id ***/
	    char *id = calloc((2 * strlen(tr->dest->id)) + 1, sizeof (char));
	    strcpy(id, tr->dest->id);
	    strcat(id, tr->dest->id);

	    struct map_item *item = hashmap_search(s_hashmap, id, STATE);
	    struct state *dest = (struct state *) item->value;

	    /*** cleanup id ***/
	    free(id);

	    struct list *l = sspace_aut->states;

	    /*** foreach sspace state (closure) ***/
	    while (l) {
		struct state *st = (struct state *) l->value;

		/*** build src lookup id ***/
		char *id = calloc(strlen(st->id) + strlen(tr->src->id) + 1, sizeof (char));
		strcpy(id, st->id);
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
		    s_tr->value = src->value;
		    
		    transition_attach(sspace_aut, s_tr);
		}
		    
		/*** cleanup id ***/
		free(id);

		l = l->next;
	    }
	}

	lt = lt->next;
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

    s_aut->states = head_insert(s_aut->states,
				list_create(s_st));
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

		s_aut->states = head_insert(s_aut->states,
					     list_create(s_next));

		s_next->final = next->final;

		/*** save pointer to silent state into next state ***/
		next->value = s_next;

		/*** and the opposite too ***/
		s_next->value = next;

		/*** connect st->silent and next->silent ***/
		struct transition *s_tr = transition_create(transition_id_create(tr_amount++));
		s_tr->src = (struct state *) st->value;
		s_tr->dest = (struct state *) next->value;

		transition_attach(s_aut, s_tr);
		
		/*** visit next state ***/
		silent_visit(next);
	    } else {
		/*** silent state already exists ***/

		/*** connect st->silent and next->silent ***/
		struct transition *s_tr = transition_create(transition_id_create(tr_amount++));
		s_tr->src = (struct state *) st->value;
		s_tr->dest = (struct state *) next->value;

		transition_attach(s_aut, s_tr);
	    }
	}
	
	
	l = l->next;
    }

    st->color = BLACK;    // not really needed for now    
}
