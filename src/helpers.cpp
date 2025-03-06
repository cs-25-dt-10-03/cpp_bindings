#include "../include/helpers.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <limits>

namespace py = pybind11;
using namespace std;


tuple<int, int> compute_aggregated_window(const vector<int>& earliest,
                                               const vector<int>& latest) {

    int global_earliest = *min_element(earliest.begin(), earliest.end());
    int min_flex = numeric_limits<int>::max();
    for (size_t i = 0; i < earliest.size(); i++) {
        int flex = latest[i] - earliest[i];
        if (flex < min_flex) {
            min_flex = flex;
        }
    }
    int aggregated_latest = global_earliest + min_flex;
    return make_tuple(global_earliest, aggregated_latest);
}

tuple<vector<double>, vector<double>> pad_profile(
    const vector<double>& min_profile,
    const vector<double>& max_profile,
    int offset, int common_length) {
    
    vector<double> padded_min(common_length, 0.0);
    vector<double> padded_max(common_length, 0.0);

    for (size_t i = 0; i < min_profile.size(); i++) {
        int pos = offset + static_cast<int>(i);
        if (pos < common_length) {
            padded_min[pos] = min_profile[i];
            padded_max[pos] = max_profile[i];
        }
    }
    return make_tuple(padded_min, padded_max);
}

tuple<vector<double>, vector<double>> align_profiles(
    const vector<vector<double>>& min_profiles,
    const vector<vector<double>>& max_profiles,
    const vector<int>& offsets) {
    int min_offset = *min_element(offsets.begin(), offsets.end());
    vector<int> norm_offsets;
    norm_offsets.reserve(offsets.size());
    for (int off : offsets) {
        norm_offsets.push_back(off - min_offset);
    }

    int common_length = 0;
    for (size_t i = 0; i < min_profiles.size(); i++) {
        int length = norm_offsets[i] + static_cast<int>(min_profiles[i].size());
        if (length > common_length) {
            common_length = length;
        }
    }

    vector<double> aggregated_min(common_length, 0.0);
    vector<double> aggregated_max(common_length, 0.0);

    for (size_t i = 0; i < min_profiles.size(); i++) {
        vector<double> padded_min, padded_max;
        tie(padded_min, padded_max) = pad_profile(min_profiles[i], max_profiles[i], norm_offsets[i], common_length);

        for (int j = 0; j < common_length; j++) {
            aggregated_min[j] += padded_min[j];
            aggregated_max[j] += padded_max[j];
        }
    }

    return make_tuple(aggregated_min, aggregated_max);
}

py::dict start_alignment_aggregate(
    const vector<vector<double>>& min_profiles,
    const vector<vector<double>>& max_profiles,
    const vector<int>& earliest,
    const vector<int>& latest,
    const vector<int>& offsets) {

    int global_earliest, aggregated_latest;
    tie(global_earliest, aggregated_latest) = compute_aggregated_window(earliest, latest);
    
    vector<double> aggregated_min, aggregated_max;
    tie(aggregated_min, aggregated_max) = align_profiles(min_profiles, max_profiles, offsets);

    py::dict result;
    result["aggregated_min"] = aggregated_min;
    result["aggregated_max"] = aggregated_max;
    result["global_earliest"] = global_earliest;
    result["aggregated_latest"] = aggregated_latest;
    result["common_length"] = static_cast<int>(aggregated_min.size());

    return result;
}