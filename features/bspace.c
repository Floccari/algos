#include "bspace.h"

int st_amount;
int tr_amount;
bool comp_set;
struct network *bs_net;
struct map_item **ct_hashmap;

struct network *compute(struct network *net, bool comp);
void step(struct state *current_bs_state);
void prune(struct network *net);
void dfs_visit(struct state *st);

char *state_id_create();
char *transition_id_create();


struct network *comp_compute(struct network *net) {
    return compute(net, true);
}

struct network *bspace_compute(struct network *net) {
    return compute(net, false);
}


struct network *compute(struct network *net, bool comp) {
    ct_hashmap = hashmap_create();

    if (comp)
	bs_net = network_create("comp_network");
    else
	bs_net = network_create("bs_network");
    
    st_amount = 0;
    tr_amount = 0;

    /*** populate bs_net with a single automaton ***/
    struct automaton *bs_aut;

    if (comp)
	bs_aut = automaton_create("comp");
    else
	bs_aut = automaton_create("bspace");	

    bs_net->automatons = head_insert(bs_net->automatons,
				       list_create(bs_aut));
    bs_net->aut_amount++;

    /*** initialize context and assign indexes to the links ***/
    struct context *c = context_create(net->aut_amount, net->lk_amount);
    struct list *l = net->automatons;
    
    for (int i = 0; i < net->aut_amount; i++) {
	struct automaton *aut = (struct automaton *) l->value;
	c->states[i] = aut->initial;

	l = l->next;
    }

    l = net->links;
    int index = 0;
    
    while (l) {
	struct link *lk = (struct link *) l->value;
	lk->index = index++;

	l = l->next;
    }
    
    memset(c->buffers, 0, sizeof (char *) * net->lk_amount);
    
    c->current_obs = get_last(net->observation);    // since it's stored in REVERSE ORDER

    /*** create initial state and insert it ***/
    struct state *st = state_create(state_id_create());
    st->context = c;

    /*** check if final ***/
    st->final = true;    // since all links are empty

    if (comp && c->current_obs)
	st->final = false;    // we still have labels left
    
    bs_aut->states = head_insert(bs_aut->states,
				   list_create(st));
    bs_aut->initial = st;

    /*** add context to hashmap ***/
    hashmap_insert(ct_hashmap, map_item_create_with_sub(context_digest(c),
						   CONTEXT, c, st));

    /*** recursive step ***/
    comp_set = comp;
    step(st);

    /*** pruning ***/
    prune(bs_net);

    /*** cleanup (does not delete contexts) ***/
    free(ct_hashmap);
    
    return bs_net;
}


void step(struct state *current_bs_state) {
    struct context *c = current_bs_state->context;

    /*** foreach current state ***/
    for (int i = 0; i < c->aut_amount; i++) {
	struct state *st = c->states[i];
	struct list *l = st->tr_out;

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

		struct list *ls = tr->act_out;

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

		if (comp_set) {
		    /*** skip transition if not applicable ***/
		    struct label *lab = NULL;
		    
		    if (new_context->current_obs)
			lab = (struct label *) new_context->current_obs->value;
		    
		    if (tr->obs) {
			if (!lab ||
			    lab->id != tr->obs->id) {
			    /*** skip transition ***/
			    context_destroy(new_context);
			    goto NEXT_TRANSITION;
			}
			
			new_context->current_obs = new_context->current_obs->prev;
			new_context->obs_index++;
		    }
		}

		/*** create the new transition and update source state ***/
		struct transition *new_tr = transition_create(transition_id_create());
		new_tr->src = current_bs_state;

		struct list *lt = list_create(new_tr);
		current_bs_state->tr_out = head_insert(current_bs_state->tr_out, lt);
	    
		new_tr->obs = tr->obs;
		new_tr->rel = tr->rel;

		/*** add the new transition to the automaton ***/
		struct automaton *bs_aut = (struct automaton *) bs_net->automatons->value;
		lt = list_create(new_tr);
		
		bs_aut->transitions = head_insert(bs_aut->transitions, lt);
	    
		/*** check if we already visited this context ***/
		struct map_item *item = context_search(ct_hashmap, new_context);
	    
		if (item) {
		    context_destroy(new_context);

		    /*** update dest state ***/
		    struct state *dest_state = (struct state *) item->subvalue;
		    new_tr->dest = dest_state;

		    lt = list_create(new_tr);
		    dest_state->tr_in = head_insert(dest_state->tr_in, lt);

		    goto NEXT_TRANSITION;    // we have already visited this context, skip this transition
		}

		/*** if we get here, we haven't visited this context ***/

		/*** create a new state ***/
		struct state *new_state = state_create(state_id_create());
		new_state->context = new_context;

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
		lt = list_create(new_state);
		bs_aut->states = head_insert(bs_aut->states, lt);

		/*** update dest state ***/
		new_tr->dest = new_state;

		lt = list_create(new_tr);
		new_state->tr_in = head_insert(new_state->tr_in, lt);

		/*** add new_context to the hashmap ***/
		hashmap_insert(ct_hashmap, map_item_create_with_sub(context_digest(new_context),
							       CONTEXT, new_context, new_state));

		/*** explore the new state completely ***/
		step(new_state);
	    }

	NEXT_TRANSITION:
	    l = l->next;
	}
    }

    // end
    return;
}

void prune(struct network *net) {
    struct automaton *aut = (struct automaton *) net->automatons->value;
    struct list *l = aut->states;

    /*** color states ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->final)
	    dfs_visit(st);

	l = l->next;
    }

    l = aut->states;

    /*** prune states ***/
    while (l) {
	struct state *st = (struct state *) l->value;

	if (st->color == WHITE) {

	    /*** remove all incoming transitions ***/
	    struct list *ls = st->tr_in;

	    while (ls) {
		struct transition *tr = (struct transition *) ls->value;

		/*** remove from source state ***/
		tr->src->tr_out = search_and_remove(tr->src->tr_out, tr);

		/*** remove from the automaton ***/
		aut->transitions = search_and_remove(aut->transitions, tr);

		/*** deallocate transition (incomplete) ***/
		free(tr);

		ls = ls->next;
	    }

	    /*** remove all outgoing transitions ***/
	    ls = st->tr_out;

	    while (ls) {
		struct transition *tr = (struct transition *) ls->value;

		/*** remove from source state ***/
		tr->dest->tr_in = search_and_remove(tr->dest->tr_in, tr);

		/*** remove from the automaton ***/
		aut->transitions = search_and_remove(aut->transitions, tr);

		/*** deallocate transition (incomplete) ***/
		free(tr);

		ls = ls->next;
	    }

	    /*** save pointer to next state ***/
	    l = l->next;
	    
	    /*** remove state from the automaton ***/
	    aut->states = search_and_remove(aut->states, st);
	    
	    /*** deallocate state (incomplete) ***/
	    free(st);
	} else
	    l = l->next;
    }
}

void dfs_visit(struct state *source) {
    source->color = GRAY;

    struct list *l = source->tr_in;    // since we are going backwards

    while (l) {
	struct transition *tr = (struct transition *) l->value;
	struct state *next = tr->src;

	if (next->color == WHITE)
	    dfs_visit(next);

	l = l->next;
    }

    source->color = BLACK;    // not really needed for now
}


char *state_id_create() {
    char *id = calloc(sizeof (char), 6);    // should use log(st_amount) instead
    sprintf(id, "%d", st_amount++);

    return id;
}

char *transition_id_create() {
    char *id = calloc(sizeof (char), 7);    // should use log(tr_amount) instead
    sprintf(id, "t%d", tr_amount++);

    return id;
}

