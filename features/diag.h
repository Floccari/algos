#ifndef DIAG_H
#define DIAG_H

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"

// assumes there's only one automaton
// and heavily modifies it
char *get_diagnosis(struct network *net);

#endif
