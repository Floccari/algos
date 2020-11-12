#include "diag.h"

int tr_amount;

void diag_error();

void phase_one(struct network *net);
void phase_two(struct network *net);
void phase_three(struct network *net);

char *get_diagnosis(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;

    tr_amount = 0;

    /*** replace initial state ***/
    struct state *init = state_create("_init");
	
    aut->states = head_insert(aut->states,
			      list_create(init));
	
    struct transition *tr = transition_create(univ_tr_id_create(tr_amount++));    

    tr->src = init;
    tr->dest = aut->initial;
    transition_attach(aut, tr);

    aut->initial = init;

    /*** replace final states ***/
    struct list *l = aut->states;
    struct state *fin = state_create("_fin");
    fin->final = true;

    aut->states = head_insert(aut->states,
			      list_create(fin));

    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->final) {
	    tr = transition_create(univ_tr_id_create(tr_amount++));
	    
	    tr->src = st;
	    tr->dest = fin;
	    transition_attach(aut, tr);

	    st->final = false;
	}

	l = l->next;
    }

    /*** compute the regexp ***/
    while (aut->transitions->next) {

	phase_one(net);    // lines 16-17 of the pseudocode
	
	phase_two(net);    // lines 18-19 of the pseudocode
	
	phase_three(net);    // lines 21-31 of the pseudocode

	/*** prevent segfaults if the input network is wrong ***/
	if (!aut->transitions)
	    diag_error();    // exits here
    }

    /*** there should be only one transition now ***/
    tr = (struct transition *) aut->transitions->value;

    if (tr->rel)
	return tr->rel->id;
    else
	return NULL;
}


void phase_one(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;
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
	    int l_in = (tr_in->rel) ? strlen(tr_in->rel->id) : 0;
	    int l_out = (tr_out->rel) ? strlen(tr_out->rel->id) : 0;

	    if (l_in + l_out > 0) {
		char *id = calloc(l_in + l_out + 1, sizeof (char));
		char *p = id;

		if (tr_in->rel)
		    strcpy(p, tr_in->rel->id);

		p += l_in;

		if (tr_out->rel)
		    strcpy(p, tr_out->rel->id);

		p += l_out;
		*p++ = '\0';

		tr->rel = label_create(id, RELEVANCE);
	    }

	    /*** replace state with new transition ***/
	    state_detach(aut, st);
	    state_destroy(st);

	    transition_attach(aut, tr);
	} else
	    l = l->next;
    }
}

void phase_two(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;
    struct map_item **tr_hashmap = hashmap_create();
    struct list *ids = NULL;

    struct list *l = aut->transitions;

    /*** foreach transition ***/
    while (l) {
	struct transition *tr1 = (struct transition *) l->value;

	/*** create look-up id ***/
	char *lookup = calloc(strlen(tr1->src->id) + strlen(tr1->dest->id) + 1, sizeof (char));
	strcpy(lookup, tr1->src->id);
	strcat(lookup, tr1->dest->id);

	struct map_item *item = hashmap_search(tr_hashmap, lookup, TRANSITION);

	if (item) {
		/*** save pointer to next transition ***/
		l = l->next;

		/*** look-up id not needed anymore ***/
		free(lookup);

		/*** recover parallel transition ***/
		struct transition *tr2 = (struct transition *) item->value;

		/*** build the appropriate label and assign it to tr2 ***/
		int l1 = (tr1->rel) ? strlen(tr1->rel->id) : 0;
		int l2 = (tr2->rel) ? strlen(tr2->rel->id) : 0;

		int extra = (tr1->rel && tr2->rel) ? 1 : 3;

		if (l1 + l2 > 0) {
		    char *id = calloc(l1 + l2 + extra + 1, sizeof (char));
		    char *p = id;

		    if (tr1->rel) {
			if (tr2->rel) {
			    strcpy(p, tr1->rel->id);
			    p += l1;
			} else {
			    *p++ = '(';
			    strcpy(p, tr1->rel->id);

			    p += l1;
			    *p++ = ')';
			    *p++ = '?';
			}
		    }


		    if (tr1->rel && tr2->rel)
			*p++ = '|';

		    if (tr2->rel) {
			if (tr1->rel) {
			    strcpy(p, tr2->rel->id);
			    p += l2;			
			} else {
			    *p++ = '(';
			    strcpy(p, tr2->rel->id);
			    
			    p += l2;
			    *p++ = ')';
			    *p++ = '?';
			}
		    }

		    *p++ = '\0';

		    tr2->rel = label_create(id, RELEVANCE);
		}
		
		/*** remove tr1 ***/
		transition_detach(aut, tr1);
	} else {
	    hashmap_insert(tr_hashmap,
			   map_item_create(lookup, TRANSITION, tr1));

	    /*** save pointer to look-up id for later cleanup ***/
	    head_insert(ids,
			list_create(lookup));

	    l = l->next;
	}
    }

    /*** cleanup ***/
    hashmap_empty(tr_hashmap);
    free(tr_hashmap);

    l = ids;

    while (l) {
	struct list *next = l->next;
	free(l->value);
	free(l);
	l = next;
    }
}

void phase_three(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;
    struct list *l = aut->states;

    /*** foreach state ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	/*** that is not initial or final ***/
	if (aut->initial != st && !st->final) {	
	    struct list *lt_in = st->tr_in;
	    char *autotr_rel = NULL;

	    /*** check if there is an auto-transition ***/
	    /*** at most one, since phase 2 removed parallel transitions ***/
	    while (lt_in) {
		struct transition *tr = (struct transition *) lt_in->value;

		if (tr->src == tr->dest) {
		    if (tr->rel)
			autotr_rel = tr->rel->id;

		    /*** remove the transition and break ***/
		    transition_detach(aut, tr);
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
		    int l_in = (tr_in->rel) ? strlen(tr_in->rel->id) : 0;
		    int l_out = (tr_out->rel) ? strlen(tr_out->rel->id) : 0;
		    int l_auto = (autotr_rel) ? strlen(autotr_rel) : 0;

		    int extra = (autotr_rel) ? 3 : 0;

		    if (l_in + l_out + l_auto > 0) {
			char *id = calloc(l_in + l_out + l_auto + extra + 1, sizeof (char));
			char *p = id;
			
			if (tr_in->rel)
			    strcpy(p, tr_in->rel->id);
			
			p += l_in;
			
			if (autotr_rel) {
			    *p++ = '(';
			    strcpy(p, autotr_rel);
			    
			    p += l_auto;
			    *p++ = ')';
			    *p++ = '*';
			}
			
			if (tr_out->rel)
			    strcpy(p, tr_out->rel->id);
			
			p += l_out;
			*p++ = '\0';

			tr->rel = label_create(id, RELEVANCE);
		    }

		    /*** attach the transition ***/
		    transition_attach(aut, tr);
		    
		    lt_out = lt_out->next;
		}

		lt_in = lt_in->next;
	    }

	    /*** save pointer to next state ***/
	    l = l->next;

	    state_detach(aut, st);
	} else
	    l = l->next;
    }
}


void diag_error() {
    fprintf(stderr, "error: this is not a behavioral space!\n");
    exit(-1);
}
