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
struct hashmap *hashmap;

#define SYMBOL_TABLE_SIZE 10000

%}


%token NETWORK AUTS EVS ARROW END AUT STS INIT OBS REL IN OUT ID DIAG
%token ERROR    

%%

program : net-decl aut-decl observation
	;

net-decl : NETWORK ID {net = network_create(lexval);
 	   	       hashmap = hashmap_create(SYMBOL_TABLE_SIZE);}
		       
	   ':' aut-list ev-list link-list END
	 ;

aut-list : AUTS ':' aut-id-list ';'
	 ;

aut-id-list : aut-id-list ',' aut-id
	    | aut-id
      	    ;

aut-id : ID {item = hashmap_strcmp_search(hashmap, lexval, AUTOMATON);

	     if (item)
		 duperror();    // exits here
	     
	     aut = automaton_create(lexval);
	     
	     hashmap_insert(hashmap,
			    map_item_create(aut->id, AUTOMATON, aut));
	     
	     net->aut_amount++;
	     tail_insert(&net->automatons,
			 list_item_create(aut));}
       ;

	    
ev-list : EVS ':' ev-id-list ';'
	| {/* eps */}
	;

ev-id-list : ev-id-list ',' ev-id
	   | ev-id
	   ;

ev-id : ID {item = hashmap_strcmp_search(hashmap, lexval, EVENT);

	    if (item)
		duperror();    // exits here
	    
	    char *ev = lexval;
	    hashmap_insert(hashmap,
			   map_item_create(ev, EVENT, NULL));
	    
	    tail_insert(&net->events,
			list_item_create(ev));}
      ;


link-list : link-list link
	  | {/* eps */}
	  ;

link : ID {item = hashmap_strcmp_search(hashmap, lexval, LINK);

       	   if (item)
	       duperror();    // exits here

	   ln = link_create(lexval);
	   hashmap_insert(hashmap,
			  map_item_create(ln->id, LINK, ln));

	   net->lk_amount++;
	   tail_insert(&net->links,
		       list_item_create(ln));}

       ID {item = hashmap_strcmp_search(hashmap, lexval, AUTOMATON);
	   free(lexval);

       	   if (!item)
	       nferror();    // exits here

	   aut = (struct automaton *) item->value;
	   
	   // ln initialized in $2
	   ln->src = aut;

	   /*tail_insert(&aut->lk_out,
		       list_item_create(ln));*/}

       ARROW ID {item = hashmap_strcmp_search(hashmap, lexval, AUTOMATON);
	         free(lexval);

        	 if (!item)
	       	    nferror();    // exits here

		 aut = (struct automaton *) item->value;

	   	 // ln initialized in $2
	   	 ln->dest = aut;
		 
	         /*tail_insert(&aut->lk_out,
			     list_item_create(ln));*/}
       ';'
     ;

aut-decl : aut-decl aut
	 | aut
	 ;

aut : AUT ID {item = hashmap_strcmp_search(hashmap, lexval, AUTOMATON);
	      free(lexval);

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

st-id-list : st-id-list ',' st
	   | st
	   ;

st : ID {// aut initialized in rule "aut"
     	 item = hashmap_strcmp_search_with_sub(hashmap, lexval, STATE, aut);

	 if (item)
	     duperror();    // exits here
	 
	 st = state_create(lexval);
	 hashmap_insert(hashmap,
			map_item_create_with_sub(st->id, STATE, st, aut));
	 
	 state_attach(aut, st);}

     regexp
	   
   | '[' ID {// aut initialized in rule "aut"
             item = hashmap_strcmp_search_with_sub(hashmap, lexval, STATE, aut);

	     if (item)
		 duperror();    // exits here
	  
	     st = state_create(lexval);
	     st->final = true;
	     hashmap_insert(hashmap,
			    map_item_create_with_sub(st->id, STATE, st, aut));
	     
	     state_attach(aut, st);}

     ']' regexp
   ;

regexp : '"' ID {// st initialized in rule "st"
       	         st->delta = lexval;}
	 '"'
       | '"' DIAG {// st initialized in rule "st"
       	           st->delta = lexval;}
	 '"'
       | {/* eps */}
       ;

init : INIT ':' ID {item = hashmap_strcmp_search_with_sub(hashmap, lexval, STATE, aut);
	            free(lexval);

       	    	    if (!item)
			nferror();    // exits here

		    // aut initialized in rule "aut"
		    aut->initial = (struct state *) item->value;}
       ';' 
     ;

tr-list : tr-list tr
	| {/* eps */}
	;

tr : ID {item = hashmap_strcmp_search(hashmap, lexval, TRANSITION);

       	   if (item)
	       duperror();    // exits here

	   tr = transition_create(lexval);
	   hashmap_insert(hashmap,
			  map_item_create(tr->id, TRANSITION, tr));}

     ID {// aut initialized in rule "aut"
     	 item = hashmap_strcmp_search_with_sub(hashmap, lexval, STATE, aut);
	 free(lexval);

	 if (!item)
	     nferror();    // exits here

	 st = (struct state *) item->value;
	 
	 // tr initialized in $2
	 tr->src = st;}

     ARROW ID {// aut initialized in rule "aut"
     	       item = hashmap_strcmp_search_with_sub(hashmap, lexval, STATE, aut);
	       free(lexval);

	       if (!item)
		   nferror();    // exits here

	       st = (struct state *) item->value;

               // tr initialized in $2
	       tr->dest = st;

	       transition_attach(aut, tr);}
     
     obs-decl rel-decl in-decl out-decl ';'
   ;

obs-decl : OBS '"' ID {item = hashmap_strcmp_search(hashmap, lexval, LABEL);
	               lab = label_create(lexval, OBSERVABILITY);
		       
	               if (item) {
			   if (item->value != (void *) OBSERVABILITY)
			       laberror();    // exits here
		       } else {
			   hashmap_insert(hashmap,
					  map_item_create(lab->id, LABEL, (void *) OBSERVABILITY));
		       }

	       	       // tr initialized in rule "tr"
	       	       tr->obs = lab;}
	   '"'
	 | {/* eps */}
	 ;

rel-decl : REL '"' ID {item = hashmap_strcmp_search(hashmap, lexval, LABEL);
	               lab = label_create(lexval, RELEVANCE);
					   
	               if (item) {
			   if (item->value != (void *) RELEVANCE)
			       laberror();    // exits here
		       } else {
			   hashmap_insert(hashmap,
					  map_item_create(lab->id, LABEL, (void *) RELEVANCE));
		       }

	       	       // tr initialized in rule "tr"
	       	       tr->rel = lab;}
	   '"'
	 | REL '"' DIAG {item = hashmap_strcmp_search(hashmap, lexval, LABEL);
	                 lab = label_create(lexval, RELEVANCE);
			 
	               	 if (item) {
			     if (item->value != (void *) RELEVANCE)
				 laberror();    // exits here
			 } else {
			     hashmap_insert(hashmap,
					    map_item_create(lab->id, LABEL, (void *) RELEVANCE));
			 }
			 
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
	        item = hashmap_strcmp_search(hashmap, lexval, EVENT);
		free(lexval);

		if (!item)
		    nferror();    // exits here
	     
		act->event = item->id;}

	    '[' ID {item = hashmap_strcmp_search(hashmap, lexval, LINK);
		    free(lexval);

	     	    if (!item)
			nferror();    // exits here

		    ln = (struct link *) item->value;

		    // aut initialized in rule "aut"
		    if (ln->dest != aut)
			lkinerror();    // exits here
		    
		    // act initialized in $2
		    act->link = ln;}
       	    ']'
          ;

out-decl : OUT '"' action-list '"'
	 | {/* eps */}
	 ;

action-list : action-list ',' action-out {// act initialized in $1
                                          // tr initialized in rule "tr"
                                          tail_insert(&tr->act_out, list_item_create(act));}

	    | action-out {// act initialized in $1
	      	          // tr initialized in rule "tr"
			  tail_insert(&tr->act_out, list_item_create(act));}

action-out : ID {act = action_create();
	         item = hashmap_strcmp_search(hashmap, lexval, EVENT);
		 free(lexval);

		 if (!item)
		     nferror();    // exits here
		 
		 act->event = item->id;}

	     '[' ID {item = hashmap_strcmp_search(hashmap, lexval, LINK);
		     free(lexval);

	     	     if (!item)
			 nferror();    // exits here
		     
		     ln = (struct link *) item->value;
		     
		     // aut initialized in rule "aut"
		     if (ln->src != aut)
			 lkouterror();    // exits here
		     
		     // act initialized in $2
		     act->link = ln;}
       	     ']'
           ;

observation : OBS ':' obs-label-list ';'
	    | {/* eps */}
	    ;

obs-label-list : obs-label-list ',' obs-label
	       | obs-label
	       ;

obs-label : ID {item = hashmap_strcmp_search(hashmap, lexval, LABEL);

      	        if (!item)
		    nferror();    // exits here

		if (item->value != (void *) OBSERVABILITY)
		    laberror();    // exits here

		tail_insert(&net->observation,
			    list_item_create(label_create(lexval, OBSERVABILITY)));}
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
