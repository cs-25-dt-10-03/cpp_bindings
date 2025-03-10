#ifndef HELPERS_H
#define HELPERS_H

#include "flexoffer.h"

#include <pybind11/pybind11.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <limits>

namespace py = pybind11;
using namespace std;

void set_time_resolution(int);
tuple<int, int> compute_aggregated_window(const vector<Flexoffer>&);
vector<int> compute_offsets_and_length(const vector<Flexoffer>&, int, int&);
Flexoffer start_alignment_aggregate(const vector<Flexoffer>&);

#endif