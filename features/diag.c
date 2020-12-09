#include "diag.h"

#include <signal.h>

extern sig_atomic_t stop;

long tr_amount;
struct state *init;
struct state *fin;

void diag_error();

void do_regexp(struct automaton *aut, bool split);
bool multiple_tr(struct automaton *aut);
void phase_one(struct automaton *aut, bool split);
void phase_two(struct automaton *aut, bool split);
void phase_three(struct automaton *aut, bool split);


char *get_diagnosis(struct automaton *aut) {
    do_regexp(aut, false);

    if (stop) {
	fprintf(stderr, "# Received termination signal during execution\n");
	fprintf(stderr, "# No partial results available\n");
	exit(-1);    // exits here
    }

    if (aut->transitions) {
	struct transition *tr = (struct transition *) aut->transitions->value;

	if (tr->rel)
	    return tr->rel->id;
	else
	    return NULL;
    } else
	return NULL;
}

struct list *get_split_diag(struct automaton *aut) {
    do_regexp(aut, true);

    return aut->transitions;
}


void do_regexp(struct automaton *aut, bool split) {
    tr_amount = 0;

    /*** add new initial state ***/
    init = state_create("_init");

    state_attach(aut, init);
	
    struct transition *tr = transition_create(univ_tr_id_create(tr_amount++));    

    tr->src = init;
    tr->dest = aut->initial;
    transition_attach(aut, tr);

    aut->initial = init;

    /*** add new final state ***/
    struct list *l = aut->states;
    fin = state_create("_fin");

    state_attach(aut, fin);

    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->final) {
	    tr = transition_create(univ_tr_id_create(tr_amount++));
	    
	    tr->src = st;
	    tr->dest = fin;
	    transition_attach(aut, tr);
	}

	l = l->next;
    }


    /*** if !split, while more than one transition ***/
    /*** else, while more than 2 states, or more than one transition with the same sub ***/
    while ((!split && aut->transitions->next) ||
	   (split && (aut->states->next->next || multiple_tr(aut)))) {

	if (!stop)
	    phase_one(aut, split);    // 16-17, split: 12-19

	if (!stop)
	    phase_two(aut, split);    // 18-19, split: 20-23

	if (!stop)
	    phase_three(aut, split);    // 21-31, split: 25-48

	if (stop)
	    break;

	/*** prevent segfaults if the input network is wrong ***/
	if (!aut->transitions)
	    diag_error();    // exits here
    }
}


bool multiple_tr(struct automaton *aut) {
    int tam = item_amount(aut->transitions);
    struct hashmap *sub_hashmap = hashmap_create(tam);
    bool empty_tr = false;

    struct list *l = aut->transitions;

    while (l) {
	struct transition *tr = (struct transition *) l->value;

	if (!tr->value) {
	    /*** check for multiple transitions with no sub ***/
	    if (empty_tr) {
		/*** cleanup ***/
		hashmap_empty(sub_hashmap, false);
		hashmap_destroy(sub_hashmap);
		
		return true;
	    } else
		empty_tr = true;
	} else {
	    /*** check for multiple transitions with the same sub ***/
	    struct state *st = (struct state *) tr->value;
	    struct map_item *item = hashmap_search(sub_hashmap, st->id, TRANSITION);

	    if (item) {
		/*** cleanup ***/
		hashmap_empty(sub_hashmap, false);
		hashmap_destroy(sub_hashmap);

		return true;
	    } else {
		/*** add transition to the hashmap ***/
		hashmap_insert(sub_hashmap,
			       map_item_create(st->id, TRANSITION, tr));
	    }
	}

	if (stop)
	    return false;
	
	l = l->next;
    }

    /*** cleanup ***/
    hashmap_empty(sub_hashmap, false);
    hashmap_destroy(sub_hashmap);    

    return false;
}

void phase_one(struct automaton *aut, bool split) {
    struct list *l = aut->states;

    /*** foreach state ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	/*** 1 tr in, 1 tr out ***/
	if (st->tr_in && !st->tr_in->next &&
	    st->tr_out && !st->tr_out->next) {

	    /*** save pointer to next state ***/
	    l = l->next;

	    /*** create new transition ***/
	    struct transition *tr = transition_create(univ_tr_id_create(tr_amount++));

	    struct transition *tr_in = (struct transition *) st->tr_in->value;
	    struct transition *tr_out = (struct transition *) st->tr_out->value;

	    tr->src = tr_in->src;
	    tr->dest = tr_out->dest;

	    /*** build the appropriate label ***/
	    tr->rel = label_cat_create(tr_in->rel, tr_out->rel);
	    
	    /*** set sub (using the relative comp state) ***/
	    if (split) {
		if (st->final)
		    tr->value = st->value;
		else
		    tr->value = tr_out->value;
	    }

	    /*** replace state with new transition ***/
	    state_detach(aut, st);
	    free(st);

	    transition_attach(aut, tr);
	} else
	    l = l->next;

	if (stop)
	    return;
    }
}

void phase_two(struct automaton *aut, bool split) {
    int tam = item_amount(aut->transitions);
    struct hashmap *tr_hashmap = hashmap_create(tam);

    struct list *l = aut->transitions;

    /*** foreach transition ***/
    while (l) {
	struct transition *tr1 = (struct transition *) l->value;

	/*** create look-up id ***/
	char *lookup;
	
	if (split && tr1->value) {
	    struct state *st = (struct state *) tr1->value;
	    lookup = calloc(strlen(tr1->src->id) + strlen(tr1->dest->id) + strlen(st->id) + 3,
				  sizeof (char));
	} else
	    lookup = calloc(strlen(tr1->src->id) + strlen(tr1->dest->id) + 2, sizeof (char));
	
	strcpy(lookup, tr1->src->id);

	char *p = lookup + strlen(tr1->src->id);
	*p++ = '#';
	*p = '\0';	
	
	strcat(lookup, tr1->dest->id);

	if (split && tr1->value) {
	    struct state *st = (struct state *) tr1->value;

	    p += strlen(tr1->dest->id);
	    *p++ = '#';
	    *p = '\0';	
	    
	    strcat(lookup, st->id);
	}

	struct map_item *item = hashmap_search(tr_hashmap, lookup, TRANSITION);

	if (item) {
		/*** save pointer to next transition ***/
		l = l->next;

		/*** look-up id not needed anymore ***/
		free(lookup);

		/*** recover parallel transition ***/
		struct transition *tr2 = (struct transition *) item->value;

		/*** build the appropriate label and assign it to tr2 ***/
		struct label *lab = label_alt_create(tr1->rel, tr2->rel);

		if (tr2->rel)
		    label_destroy(tr2->rel);
		
		tr2->rel = lab;
		
		/*** remove tr1 ***/
		transition_detach(aut, tr1);
		transition_destroy(tr1);
	} else {
	    hashmap_insert(tr_hashmap,
			   map_item_create(lookup, TRANSITION, tr1));

	    l = l->next;
	}

	if (stop)
	    return;
    }

    /*** cleanup ***/
    hashmap_empty(tr_hashmap, true);
    hashmap_destroy(tr_hashmap);
}

void phase_three(struct automaton *aut, bool split) {
    struct list *l = aut->states;

    /*** foreach state ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	/*** that is not _init or _fin ***/
	if (st != init && st != fin) {
	    struct list *lt_in = st->tr_in;
	    struct label *autotr = NULL;

	    /*** check if there is an auto-transition ***/
	    /*** at most one, since phase 2 removed parallel transitions ***/
	    while (lt_in) {
		struct transition *tr = (struct transition *) lt_in->value;

		if (tr->src == tr->dest) {
		    if (tr->rel)
			autotr = label_copy(tr->rel);

		    /*** remove the transition and break ***/
		    transition_detach(aut, tr);
		    transition_destroy(tr);
		    break;

		} else
		    lt_in = lt_in->next;
	    }

	    lt_in = st->tr_in;

	    /*** foreach incoming transition ***/
	    while (lt_in) {
		struct transition *tr_in = (struct transition *) lt_in->value;
		
		struct list *lt_out = st->tr_out;

		/*** foreach outgoing transition ***/
		while (lt_out) {
		    struct transition *tr_out = (struct transition *) lt_out->value;

		    /*** create a new transition ***/
		    struct transition *tr = transition_create(univ_tr_id_create(tr_amount++));
		    tr->src = tr_in->src;
		    tr->dest = tr_out->dest;

		    /*** build the appropriate label ***/
		    tr->rel = label_cat_auto_create(tr_in->rel, autotr, tr_out->rel);

		    /*** set sub (using the relative comp state) ***/
		    if (split) {
			if (st->final && tr->dest == fin && !tr_out->value)
			    tr->value = st->value;
			else
			    tr->value = tr_out->value;
		    }

		    /*** attach the transition ***/
		    transition_attach(aut, tr);
		    
		    if (stop)
			break;

		    lt_out = lt_out->next;
		}

		if (stop)
		    break;
		
		lt_in = lt_in->next;
	    }

	    /*** save pointer to next state ***/
	    l = l->next;

	    state_detach(aut, st);
	    free(st);
	} else
	    l = l->next;

	if (stop)
	    return;
    }
}


void diag_error() {
    fprintf(stderr, "error: this is not a behavioral space!\n");
    exit(-1);
}
