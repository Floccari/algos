#ifndef DCTOR_H
#define DCTOR_H

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"

#include "diag.h"

struct automaton *get_diagnosticator(struct automaton *bspace_aut);

#endif
