#include "data_structures/list.h"
#include "data_structures/hashmap.h"
#include "data_structures/network.h"

#include "features/bspace.h"
#include "features/diag.h"
#include "features/dctor.h"

#include <signal.h>

extern struct network *net;
extern struct hashmap *hashmap;
extern void yyparse();
extern void yyset_in(FILE *in);


sig_atomic_t stop= 0;

void handler(int signal_number) {
    stop = 1;
}


int main(int argc, char **argv) {
    FILE *fc;

    /*** signal handler ***/
    struct sigaction sa;
    memset(&sa, 0, sizeof (struct sigaction));
    
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);

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
	printf("\n");
	printf("\tbspace\tCompute the behavioral space of the network\n");
	printf("\tcomp\tCompute a behavioral subspace of the network given an observation\n");
	printf("\tdiag\tOutput a diagnosis given a behavioral subspace\n");
	printf("\tdctor\tBuild the diagnosticator of a network given its behavioral space\n");
	printf("\tdcdiag\tOutput a diagnosis given a diagnosticator and an observation\n");
	
	exit(0);
    }

    /*** set input file if specified ***/
    if (argc == 3) {
	fc = fopen(argv[2], "r");
	yyset_in(fc);
    }

    /*** parse input network ***/
    yyparse();
    hashmap_empty(hashmap, false);
    hashmap_destroy(hashmap);

    /*** actions ***/
    if (strcmp(argv[1], "test") == 0) {

	network_serialize(stdout, net);
	exit(0);
	
    } else if (strcmp(argv[1], "dot") == 0) {

	network_to_dot(stdout, net);
	exit(0);
	
    } else if (strcmp(argv[1], "bspace") == 0) {

	struct network *bs_net = bspace_compute(net);
	bs_net->observation = net->observation;

	if (stop) {
	    fprintf(stdout, "# Received termination signal during execution\n");
	    fprintf(stdout, "# Showing partial results\n");
	    fprintf(stdout, "\n");
	}
	
	network_print_subs(stdout, net, bs_net, false);
	network_serialize(stdout, bs_net);
	exit(0);
	
    } else if (strcmp(argv[1], "comp") == 0) {

	struct network *c_net = comp_compute(net);
	c_net->observation = net->observation;

	if (stop) {
	    fprintf(stdout, "# Received termination signal during execution\n");
	    fprintf(stdout, "# Showing partial results\n");
	    fprintf(stdout, "\n");
	}
	
	network_print_subs(stdout, net, c_net, true);
	network_serialize(stdout, c_net);
	exit(0);
	
    } else if (strcmp(argv[1], "diag") == 0) {

	struct automaton *aut = (struct automaton *) net->automatons->value;
	char *diagnosis = get_diagnosis(aut);
	fprintf(stdout, "%s\n", diagnosis);
	exit(0);
	
    } else if (strcmp(argv[1], "dctor") == 0) {

	struct network *dctor = network_create("dctor_net");
	struct automaton *in = (struct automaton *) net->automatons->value;
	dctor->automatons = head_insert(dctor->automatons,
					list_create(get_diagnosticator(in)));
	dctor->observation = net->observation;

	if (stop) {
	    fprintf(stdout, "# Received termination signal during execution\n");
	    fprintf(stdout, "# Showing partial results\n");
	    fprintf(stdout, "\n");
	}

	network_serialize(stdout, dctor);
	exit(0);
	
    }  else if (strcmp(argv[1], "dcdiag") == 0) {

	char *diagnosis = diagnosticate((struct automaton *) net->automatons->value,
				       net->observation);
	fprintf(stdout, "%s\n", diagnosis);
	exit(0);
	
    } else 
	printf("Unknown action\n");
    
    return 0;
}
