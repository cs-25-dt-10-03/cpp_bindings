#ifndef ALIGNMENTS_H
#define ALIGNMENTS_H

#include "helpers.h" 
#include "flexoffer.h"

#include <vector>
#include <tuple>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <limits>
#include <vector>


Flexoffer start_alignment_aggregate(const vector<Flexoffer>&);
Flexoffer balance_alignment_aggregate(const vector<Flexoffer>&, int num_candidates);




#endif