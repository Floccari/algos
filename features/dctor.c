#include "dctor.h"

struct automaton *s_aut;
struct list *visited;
int tr_amount;

void silent_visit(struct state *st);

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

		/*** save pointer to silent state into next state ***/
		next->value = s_next;

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
