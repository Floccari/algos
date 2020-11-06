%{

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"
    
extern int line;
extern char *lexval;
extern char *yytext;
extern int yylex();
extern char *newstring(char *str);

void yyerror();
void duperror();
void nferror();
void lkinerror();
void lkouterror();
void laberror();

void yyset_in(FILE *in);

struct network *net;
struct automaton *aut;
struct link *ln = NULL;
struct action *act = NULL;
struct state *st;
struct transition *tr = NULL;
struct label *lab = NULL;

struct map_item *item = NULL;
struct map_item **hashmap;

%}


%token NETWORK AUTS EVS ARROW END AUT STS INIT OBS REL IN OUT ID
%token ERROR    

%%

program : net-decl aut-decl observation
	;

net-decl : NETWORK ID {net = network_create(lexval);}
	   ':' aut-list ev-list link-list END
	 ;

aut-list : AUTS ':' aut-id-list ';'
	 ;

aut-id-list : aut-id ',' aut-id-list
	    | aut-id
      	    ;

aut-id : ID {item = hashmap_search(hashmap, lexval, AUTOMATON);

	     if (item)
		 duperror();    // exits here
	     
	     aut = automaton_create(lexval);
	     hashmap_insert(hashmap,
			    map_item_create(aut->id, AUTOMATON, aut));
	     
	     struct list *l = list_create(aut);
	     net->aut_amount++;
	     net->automatons = head_insert(net->automatons, l);}
       ;

	    
ev-list : EVS ':' ev-id-list ';'
	| {/* eps */}
	;

ev-id-list : ev-id ',' ev-id-list
	   | ev-id
	   ;

ev-id : ID {item = hashmap_search(hashmap, lexval, EVENT);

	    if (item)
		duperror();    // exits here
	    
	    char *ev = lexval;
	    hashmap_insert(hashmap,
			   map_item_create(ev, EVENT, NULL));
	    
	    struct list *l = list_create(ev);
	    net->events = head_insert(net->events, l);}
      ;


link-list : link link-list
	  | {/* eps */}
	  ;

link : ID {item = hashmap_search(hashmap, lexval, LINK);

       	   if (item)
	       duperror();    // exits here

	   ln = link_create(lexval);
	   hashmap_insert(hashmap,
			  map_item_create(ln->id, LINK, ln));

	   struct list *l = list_create(ln);
	   net->lk_amount++;
	   net->links = head_insert(net->links, l);}

       ID {item = hashmap_search(hashmap, lexval, AUTOMATON);

       	   if (!item)
	       nferror();    // exits here

	   aut = (struct automaton *) item->value;
	   
	   // ln initialized in $2
	   ln->src = aut;

	   /*struct list *l = list_create(ln);
	     aut->lk_out = head_insert(aut->lk_out, l);*/}

       ARROW ID {item = hashmap_search(hashmap, lexval, AUTOMATON);

        	 if (!item)
	       	    nferror();    // exits here

		 aut = (struct automaton *) item->value;

	   	 // ln initialized in $2
	   	 ln->dest = aut;
		 
		 /*struct list *l = list_create(ln);
		   aut->lk_in = head_insert(aut->lk_in, l);*/}
       ';'
     ;

aut-decl : aut aut-decl
	 | aut
	 ;

aut : AUT ID {item = hashmap_search(hashmap, lexval, AUTOMATON);

      	      if (!item)
		  nferror();    // exits here

	      if ((bool) item->subvalue)
		  duperror();    //exits here
	      
	      item->subvalue = (void *) 1;
	      aut = (struct automaton *) item->value;}

      ':' st-list init tr-list END
    ;

st-list : STS ':' st-id-list ';'
	;

st-id-list : st ',' st-id-list
	   | st
	   ;

st : ID {// aut initialized in rule "aut"
     	 item = hashmap_search_with_sub(hashmap, lexval, STATE, aut);

	 if (item)
	     duperror();    // exits here
	 
	 st = state_create(lexval);
	 hashmap_insert(hashmap,
			map_item_create_with_sub(st->id, STATE, st, aut));
	 
	 struct list *l = list_create(st);
	 aut->states = head_insert(aut->states, l);}
	   
   | '(' ID {// aut initialized in rule "aut"
             item = hashmap_search_with_sub(hashmap, lexval, STATE, aut);

	     if (item)
		 duperror();    // exits here
	  
	     st = state_create(lexval);
	     st->final = true;
	     hashmap_insert(hashmap,
			    map_item_create_with_sub(st->id, STATE, st, aut));
	     
	     struct list *l = list_create(st);
	     aut->states = head_insert(aut->states, l);}

     ')'
   ;

init : INIT ':' ID {item = hashmap_search_with_sub(hashmap, lexval, STATE, aut);

       	    	    if (!item)
			nferror();    // exits here

		    // aut initialized in rule "aut"
		    aut->initial = (struct state *) item->value;}
       ';' 
     ;

tr-list : tr tr-list
	| {/* eps */}
	;

tr : ID {item = hashmap_search(hashmap, lexval, TRANSITION);

       	   if (item)
	       duperror();    // exits here

	   tr = transition_create(lexval);
	   hashmap_insert(hashmap,
			  map_item_create(tr->id, TRANSITION, tr));

	   struct list *l = list_create(tr);

	   // aut initialized in rule "aut"		 
	   aut->transitions = head_insert(aut->transitions, l);}

     ID {// aut initialized in rule "aut"
     	 item = hashmap_search_with_sub(hashmap, lexval, STATE, aut);

	 if (!item)
	     nferror();    // exits here

	 st = (struct state *) item->value;
	 
	 // tr initialized in $2
	 tr->src = st;

	 struct list *l = list_create(tr);
	 st->tr_out = head_insert(st->tr_out, l);}

     ARROW ID {// aut initialized in rule "aut"
     	       item = hashmap_search_with_sub(hashmap, lexval, STATE, aut);

	       if (!item)
		   nferror();    // exits here

	       st = (struct state *) item->value;

               // tr initialized in $2
	       tr->dest = (struct state *) item->value;

	       struct list *l = list_create(tr);
	       st->tr_in = head_insert(st->tr_in, l);}
     
     obs-decl rel-decl in-decl out-decl ';'
   ;

obs-decl : OBS '"' ID {item = hashmap_search(hashmap, lexval, LABEL);

	               if (item) {
			   lab = (struct label *) item->value;

			   if (lab->type != OBSERVABILITY)
			       laberror();    // exits here
		       } else
			   lab = label_create(lexval, OBSERVABILITY);

		       hashmap_insert(hashmap,
				      map_item_create(lab->id, LABEL, lab));

	       	       // tr initialized in rule "tr"
	       	       tr->obs = lab;}
	   '"'
	 | {/* eps */}
	 ;

rel-decl : REL '"' ID {item = hashmap_search(hashmap, lexval, LABEL);

	               if (item) {
			   lab = (struct label *) item->value;

			   if (lab->type != RELEVANCE)
			       laberror();    // exits here
		       } else
			   lab = label_create(lexval, RELEVANCE);

	               hashmap_insert(hashmap,
				      map_item_create(lab->id, LABEL, lab));

	       	       // tr initialized in rule "tr"
	       	       tr->rel = lab;}
	   '"'
	 | {/* eps */}
	 ;

in-decl : IN '"' action-in {// tr initialized in rule "tr"
	     	 	    // act initialized in rule "action"
	     	 	    tr->act_in = act;}
	  '"' 
	| {/* eps */}
	;

action-in : ID {act = action_create();
	        item = hashmap_search(hashmap, lexval, EVENT);

		if (!item)
		    nferror();    // exits here
	     
		act->event = item->id;}

	    '(' ID {item = hashmap_search(hashmap, lexval, LINK);

	     	    if (!item)
			nferror();    // exits here

		    ln = (struct link *) item->value;

		    // aut initialized in rule "aut"
		    if (ln->dest != aut)
			lkinerror();    // exits here
		    
		    // act initialized in $2
		    act->link = ln;}
       	    ')'
          ;

out-decl : OUT '"' action-list '"'
	 | {/* eps */}
	 ;

action-list : action-out {// act initialized in $1
                          struct list *l = list_create(act);

			  // tr initialized in rule "tr"
			  tr->act_out = head_insert(tr->act_out, l);}

	      ',' action-list

	    | action-out {// act initialized in $1
	      	          struct list *l = list_create(act);

			  // tr initialized in rule "tr"
			  tr->act_out = head_insert(tr->act_out, l);}

action-out : ID {act = action_create();
	         item = hashmap_search(hashmap, lexval, EVENT);

		 if (!item)
		     nferror();    // exits here
		 
		 act->event = item->id;}

	     '(' ID {item = hashmap_search(hashmap, lexval, LINK);

	     	     if (!item)
			 nferror();    // exits here
		     
		     ln = (struct link *) item->value;
		     
		     // aut initialized in rule "aut"
		     if (ln->src != aut)
			 lkouterror();    // exits here
		     
		     // act initialized in $2
		     act->link = ln;}
       	     ')'
           ;

observation : OBS ':' obs-label-list ';'
	    | {/* eps */}
	    ;

obs-label-list : obs-label ',' obs-label-list
	       | obs-label
	       ;

obs-label : ID {item = hashmap_search(hashmap, lexval, LABEL);

      	        if (!item)
		    nferror();    // exits here

		lab = (struct label *) item->value;
		
		if (lab->type != OBSERVABILITY)
		    laberror();    // exits here

		struct list *l = list_create(lab);
		net->observation = head_insert(net->observation, l);}
	  ;

%%

void yyerror() {
    fprintf(stderr, "line %d: sytax error on symbol \"%s\"\n", line, yytext);
    exit(-1);
}

void duperror() {
    fprintf(stderr, "line %d: \"%s\" already defined\n", line, yytext);
    exit(-1);
}

void nferror() {
    fprintf(stderr, "line %d: \"%s\" undefined\n", line, yytext);
    exit(-1);
}

void lkinerror() {
    fprintf(stderr, "line %d: link \"%s\" is not an input to this automaton\n", line, yytext);
    exit(-1);
}

void lkouterror() {
    fprintf(stderr, "line %d: link \"%s\" is not an output to this automaton\n", line, yytext);
    exit(-1);
}

void laberror() {
    fprintf(stderr, "line %d: label \"%s\" has wrong type\n", line, yytext);
    exit(-1);
}
