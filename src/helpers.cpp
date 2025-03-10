#include "../include/helpers.h"
#include "../include/flexoffer.h"

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

int TIME_RESOLUTION = 3600; //default
void set_time_resolution(int resolution) { //Overwrite
    if (resolution != 3600 && resolution != 900) {
        throw invalid_argument("Invalid time resolution! Must be 60 min or 15 min");
    }
    TIME_RESOLUTION = resolution;
}

tuple<int, int> compute_aggregated_window(const vector<Flexoffer>& flex_offers) {

    time_t global_earliest = numeric_limits<int>::max();
    int min_flex = numeric_limits<int>::max();
    
    for (const auto& fo : flex_offers) {
        time_t earliest = fo.get_est();
        time_t latest = fo.get_lst();
        int flex = latest - earliest;

        if (earliest < global_earliest) {
            global_earliest = earliest;
        }
        if (flex < min_flex) {
            min_flex = flex;
        }
    }

    int aggregated_latest = global_earliest + min_flex;
    return make_tuple(global_earliest, aggregated_latest);
}

vector<int> compute_offsets_and_length(const vector<Flexoffer>& flex_offers, time_t global_earliest, int& common_length) {
    vector<int> offsets;
    offsets.reserve(flex_offers.size());
    common_length = 0;

    for (const auto& fo : flex_offers) {
        int offset = static_cast<int>((fo.get_est() - global_earliest) / TIME_RESOLUTION);
        offsets.push_back(offset);
        common_length = max(common_length, offset + fo.get_duration());
    }

    return offsets;
}

Flexoffer start_alignment_aggregate(const vector<Flexoffer>& flex_offers) {

    time_t global_earliest, aggregated_latest;
    tie(global_earliest, aggregated_latest) = compute_aggregated_window(flex_offers);

    int common_length;
    vector<int> offsets = compute_offsets_and_length(flex_offers, global_earliest, common_length);

    vector<TimeSlice> aggregated_profile(common_length, TimeSlice(0.0, 0.0));

    for (size_t i = 0; i < flex_offers.size(); i++) {
        int offset = offsets[i];
        const auto& profile = flex_offers[i].get_profile();

        for (size_t j = 0; j < profile.size(); j++) {
            int index = offset + j;
            aggregated_profile[index].min_power += profile[j].min_power;
            aggregated_profile[index].max_power += profile[j].max_power;
        }
    }

    return Flexoffer(
        -1,
        global_earliest,
        aggregated_latest,
        aggregated_latest,
        aggregated_profile,
        common_length,
        0.0,
        0.0
    );
}