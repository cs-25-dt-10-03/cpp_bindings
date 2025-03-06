#ifndef HELPERS_H
#define HELPERS_H

#include <pybind11/pybind11.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <limits>

namespace py = pybind11;
using namespace std;

tuple<int, int> compute_aggregated_window(const vector<int>&, const vector<int>&);
tuple<vector<double>, vector<double>> pad_profile(const vector<double>&, const vector<double>&, int, int);
tuple<vector<double>, vector<double>> align_profiles(const vector<vector<double>>&, const vector<vector<double>>&,const vector<int>&);
py::dict start_alignment_aggregate(
    const vector<vector<double>>&,
    const vector<vector<double>>&,
    const vector<int>&,
    const vector<int>&,
    const vector<int>&);

#endif