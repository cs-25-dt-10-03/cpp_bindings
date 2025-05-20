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

struct FO_Node {
    Flexoffer offer;
    int index1;
    int index2;
    int depth;

    FO_Node(Flexoffer fo, int i1 = -1, int i2 = -1, int d = 0)
        : offer(fo), index1(i1), index2(i2), depth(d) {}
};

Flexoffer merge_best_offset(const Flexoffer& a, const Flexoffer& b, int num_candidates);
double balance_cost(const Flexoffer& a, const Flexoffer& b, int offset);
Flexoffer start_alignment_aggregate(const vector<Flexoffer>&);
Flexoffer balance_alignment_aggregate(const vector<Flexoffer>&, int num_candidates);
Flexoffer balance_alignment_tree_merge(const vector<Flexoffer>, int num_candidates);



#endif