#include "bspace.h"

#include <signal.h>

extern sig_atomic_t stop;

long st_amount;
long tr_amount;
bool comp_set;
struct automaton *bs_aut;
struct hashmap *ct_hashmap;

struct automaton *compute(struct network *net, bool comp);
void step(struct state *current_bs_state);
void prune(struct automaton *net);
void dfs_visit(struct state *st);


struct automaton *comp_compute(struct network *net) {
    return compute(net, true);
}

struct automaton *bspace_compute(struct network *net) {
    return compute(net, false);
}


struct automaton *compute(struct network *net, bool comp) {
    size_t ct_amount = context_amount_estimate(net);
    ct_hashmap = hashmap_create(ct_amount);

    st_amount = 0;
    tr_amount = 0;

    if (comp)
	bs_aut = automaton_create("comp");
    else
	bs_aut = automaton_create("bspace");

    /*** initialize context and assign indexes to the links ***/
    struct context *c = context_create(net->aut_amount, net->lk_amount);
    struct list_item *l = net->automatons.head;
    
    for (int i = 0; i < net->aut_amount; i++) {
	struct automaton *aut = (struct automaton *) l->value;
	c->states[i] = aut->initial;

	l = l->next;
    }

    l = net->links.head;
    int index = 0;
    
    while (l) {
	struct link *lk = (struct link *) l->value;
	lk->index = index++;

	l = l->next;
    }
    
    memset(c->buffers, 0, sizeof (char *) * net->lk_amount);
    
    c->id = context_id_create(c);
    c->current_obs = net->observation.head;

    /*** create initial state and insert it ***/
    struct state *st = state_create(state_id_create(st_amount++));
    st->value = c;

    st->final = true;    // since all links are empty

    if (comp && c->current_obs)
	st->final = false;    // we still have labels left
    
    state_attach(bs_aut, st);
    bs_aut->initial = st;

    /*** add context to hashmap ***/
    hashmap_insert(ct_hashmap, map_item_create_with_sub(c->id, CONTEXT, c, st));

    /*** recursive step ***/
    if (!stop) {
	comp_set = comp;
	step(st);
    }

    /*** pruning ***/
    if (!stop)
	prune(bs_aut);

    /*** cleanup (does not delete contexts) ***/
    hashmap_empty(ct_hashmap, false);
    hashmap_destroy(ct_hashmap);
    
    return bs_aut;
}


void step(struct state *current_bs_state) {
    struct context *c = (struct context *) current_bs_state->value;

    /*** foreach current state ***/
    for (int i = 0; i < c->aut_amount; i++) {
	struct state *st = c->states[i];
	struct list_item *l = st->tr_out.head;

	/*** foreach outgoing transition ***/	
	while (l) {
	    struct transition *tr = (struct transition *) l->value;
	    struct action *a = tr->act_in;

	    /*** check input condition ***/
	    if (!a || c->buffers[a->link->index] == a->event) {
		struct context *new_context = context_copy(c);

		/*** update state ***/
		new_context->states[i] = tr->dest;

		if (a) {
		    /*** update input link ***/
		    new_context->buffers[a->link->index] = NULL;
		}

		struct list_item *ls = tr->act_out.head;

		/*** update output links, abort if one of them is full ***/
		while (ls) {
		    struct action *ac = (struct action *) ls->value;
		
		    if (new_context->buffers[ac->link->index]) {
			context_destroy(new_context);
			goto NEXT_TRANSITION;    // one of the links is full, skip this transition
		    }

		    new_context->buffers[ac->link->index] = ac->event;

		    ls = ls->next;
		}

		/*** set context id ***/
		new_context->id = context_id_create(new_context);

		if (comp_set) {
		    /*** skip transition if not applicable ***/
		    struct label *lab = NULL;
		    
		    if (new_context->current_obs)
			lab = (struct label *) new_context->current_obs->value;
		    
		    if (tr->obs) {
			if (!lab || strcmp(lab->id, tr->obs->id) != 0) {
			    /*** skip transition ***/
			    context_destroy(new_context);
			    goto NEXT_TRANSITION;
			}
			
			new_context->current_obs = new_context->current_obs->next;
			new_context->obs_index++;
		    }
		}

		/*** create the new transition and set source ***/
		struct transition *new_tr = transition_create(transition_id_create(tr_amount++));
		new_tr->src = current_bs_state;
		new_tr->obs = label_copy(tr->obs);
		new_tr->rel = label_copy(tr->rel);

		/*** check if we already visited this context ***/
		struct map_item *item = hashmap_strcmp_search(ct_hashmap, new_context->id, CONTEXT);
	    
		if (item) {
		    context_destroy(new_context);

		    /*** set destination and attach transition ***/
		    struct state *dest_state = (struct state *) item->subvalue;
		    new_tr->dest = dest_state;

		    transition_attach(bs_aut, new_tr);
		    
		    goto NEXT_TRANSITION;    // we have already visited this context, skip this transition
		}

		/*** if we get here, we haven't visited this context ***/

		/*** create a new state ***/
		struct state *new_state = state_create(state_id_create(st_amount++));
		new_state->value = new_context;

		/*** check if new state is final ***/
		new_state->final = true;

		for (int i = 0; i < new_context->lk_amount; i++)
		    if (new_context->buffers[i]) {
			new_state->final = false;
			break;
		    }

		if (comp_set && new_context->current_obs)
		    new_state->final = false;    // we still have labels left

		/*** add new state to the automaton ***/
		state_attach(bs_aut, new_state);

		/*** set destination and attach transition ***/
		new_tr->dest = new_state;
		transition_attach(bs_aut, new_tr);

		/*** add new_context to the hashmap ***/
		hashmap_insert(ct_hashmap, map_item_create_with_sub(new_context->id,
								    CONTEXT, new_context, new_state));

		/*** explore the new state completely ***/
		if (!stop)
		    step(new_state);
	    }

	NEXT_TRANSITION:
	    l = l->next;

	    if (stop)
		break;
	}

	if (stop)
	    break;
    }

    // end
    return;
}

void prune(struct automaton *aut) {
    struct list_item *l = aut->states.head;

    /*** init sttr_hashmap ***/
    sttr_hashmap_fill(aut);

    /*** color states ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->final)
	    dfs_visit(st);

	l = l->next;

	if (stop)
	    break;
    }

    l = aut->states.head;

    /*** prune states ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->color == WHITE && aut->initial != st) {

	    /*** save pointer to next state ***/
	    l = l->next;

	    state_detach(aut, st);
	    context_destroy(st->value);
	    free(st->id);
	    free(st);
	} else
	    l = l->next;

	if (stop)
	    break;
    }
}

void dfs_visit(struct state *source) {
    source->color = GRAY;

    struct list_item *l = source->tr_in.head;    // since we are going backwards

    while (l) {
	struct transition *tr = (struct transition *) l->value;
	struct state *next = tr->src;

	if (next->color == WHITE)
	    if (!stop)
		dfs_visit(next);

	l = l->next;

	if (stop)
	    break;
    }

    source->color = BLACK;    // not really needed for now
}
