#ifndef BSPACE_H
#define BSPACE_H

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"

struct automaton *comp_compute(struct network *net);

struct automaton *bspace_compute(struct network *net);

#endif
