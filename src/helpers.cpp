#include "../include/helpers.h"
#include "../include/flexoffer.h"
#include "../include/config.h"

#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>

using namespace std;

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

double compute_total_flex(const Flexoffer& fo) {
    double time_flex = static_cast<double>(fo.get_lst() - fo.get_est());
    double amount_flex = 0.0;
    for (const auto& s : fo.get_profile()) {
        amount_flex += (s.max_power - s.min_power);
    }
    return time_flex * amount_flex;
}


double abs_balance(const Flexoffer& fo) {
    double total = 0.0;
    for (const auto& s : fo.get_profile()) {
        double avg = 0.5 * (s.min_power + s.max_power);
        total += fabs(avg);
    }
    return total;
}

int get_least_flexible_index(const vector<Flexoffer>& flex_offers, const vector<bool>& used) {
    int min_index = -1;
    double min_total_flex = numeric_limits<double>::max();
    for (size_t i = 0; i < flex_offers.size(); ++i) {
        if (used[i]) continue;
        double tf = compute_total_flex(flex_offers[i]);
        if (tf < min_total_flex) {
            min_total_flex = tf;
            min_index = static_cast<int>(i);
        }
    }
    return min_index;
}


Flexoffer aggregate_two(const Flexoffer& a, const Flexoffer& b, int offset) {
    // New aggregate length is the maximum of a's length and (offset + b's length)
    int new_length = max(a.get_duration(), offset + b.get_duration());
    vector<TimeSlice> new_profile(new_length, TimeSlice(0.0, 0.0));

    // Add profile of 'a'
    const auto& profA = a.get_profile();
    for (size_t i = 0; i < profA.size(); ++i) {
        new_profile[i].min_power += profA[i].min_power;
        new_profile[i].max_power += profA[i].max_power;
    }
    // Add profile of 'b', shifted by offset
    const auto& profB = b.get_profile();
    for (size_t i = 0; i < profB.size(); ++i) {
        int pos = offset + static_cast<int>(i);
        if (pos < new_length) {
            new_profile[pos].min_power += profB[i].min_power;
            new_profile[pos].max_power += profB[i].max_power;
        }
    }
    
    // New aggregate retains the original start time.
    time_t new_start = a.get_est();
    time_t new_end = new_start + new_length * TIME_RESOLUTION;
    
    // Combine overall allocations.
    double new_min_alloc = a.get_min_overall_alloc() + b.get_min_overall_alloc();
    double new_max_alloc = a.get_max_overall_alloc() + b.get_max_overall_alloc();
    
    return Flexoffer(-1, new_start, new_end, new_end, new_profile, new_length, new_min_alloc, new_max_alloc);
}