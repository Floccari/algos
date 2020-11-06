#include "diag.h"

int tr_amount;

char *transition_id_create();

char *get_diagnosis(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;

    tr_amount = 0;

    /*** replace initial state ***/
    if (aut->initial->tr_in) {
	struct state *init = state_create("_init");
	
	aut->states = head_insert(aut->states,
				  list_create(init));
	
	struct transition *tr = transition_create(transition_id_create(tr_amount++));    

	tr->src = init;
	tr->dest = aut->initial;
	transition_attach(aut, tr);

	aut->initial = init;
    }

    struct list *l = aut->states;
    struct state *fin = state_create("_fin");
    fin->final = true;

    aut->states = head_insert(aut->states,
			      list_create(fin));

    /*** replace final state ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->final) {
	    struct transition *tr = transition_create(transition_id_create(tr_amount++));
	    
	    tr->src = st;
	    tr->dest = fin;
	    transition_attach(aut, tr);

	    st->final = false;
	}
    }

    // TODO: compute the regexp

    return NULL;
}

