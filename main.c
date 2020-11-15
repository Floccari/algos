#include "data_structures/list.h"
#include "data_structures/hashmap.h"
#include "data_structures/network.h"

#include "features/bspace.h"
#include "features/diag.h"
#include "features/dctor.h"

extern struct network *net;
extern struct map_item **hashmap;
extern void yyparse();
extern void yyset_in(FILE *in);


int main(int argc, char **argv) {
    FILE *fc;
    hashmap = hashmap_create();

    /*** help message ***/
    if (argc < 2 || argc > 3 ||
	strcmp(argv[1], "help") == 0 ||
	strcmp(argv[1], "--help") == 0 ||
	strcmp(argv[1], "-h") == 0) {

	printf("Usage:\n");
	printf("\t%s action [file_in]\n", argv[0]);
	printf("\n");
	printf("Actions:\n");
	printf("\thelp\tShow this help message\n");
	printf("\ttest\tTest network loading and serialization\n");
	printf("\tdot\tOutput dot representation of the network\n");
	printf("\tbspace\tCompute the behavioral space of the network\n");
	printf("\tcomp\tCompute a behavioral subspace of the network given an observation\n");
	printf("\tdiag\tOutput a diagnosis given a behavioral subspace\n");
	
	exit(0);
    }

    /*** set input file if specified ***/
    if (argc == 3) {
	fc = fopen(argv[2], "r");
	yyset_in(fc);
    }

    /*** actions ***/
    if (strcmp(argv[1], "test") == 0) {
	yyparse();

	network_serialize(stdout, net);

	exit(0);
    } else if (strcmp(argv[1], "dot") == 0) {
	yyparse();

	network_to_dot(stdout, net);

	exit(0);
    } else if (strcmp(argv[1], "bspace") == 0) {
	yyparse();

	struct network *bs_net = bspace_compute(net);
	network_print_subs(stdout, net, bs_net, false);
	network_serialize(stdout, bs_net);

	exit(0);
    } else if (strcmp(argv[1], "comp") == 0) {
	yyparse();

	struct network *c_net = comp_compute(net);
	network_print_subs(stdout, net, c_net, true);
	network_serialize(stdout, c_net);

	exit(0);
    } else if (strcmp(argv[1], "diag") == 0) {
	yyparse();

	struct automaton *aut = (struct automaton *) net->automatons->value;
	char *diagnosis = get_diagnosis(aut);
	fprintf(stdout, "%s\n", diagnosis);

	exit(0);
    } else 
	printf("Unknown action\n");
    
    return 0;
}
