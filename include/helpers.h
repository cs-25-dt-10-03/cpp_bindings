#ifndef HELPERS_H
#define HELPERS_H

#include "flexoffer.h"
#include "config.h"
#include <vector>
#include <tuple>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <limits>

tuple<int, int> compute_aggregated_window(const vector<Flexoffer>&);
vector<int> compute_offsets_and_length(const vector<Flexoffer>&, time_t, int&);
double compute_total_flex(const Flexoffer&);
double abs_balance(const Flexoffer&);
int get_least_flexible_index(const vector<Flexoffer>&, const vector<bool>&);
Flexoffer aggregate_two(const Flexoffer&, const Flexoffer&, int);

#endif