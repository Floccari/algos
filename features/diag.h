#ifndef DIAG_H
#define DIAG_H

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"

// heavily modifies aut
char *get_diagnosis(struct automaton *aut);

// heavily modifies aut
struct list *get_split_diag(struct automaton *aut);

#endif
