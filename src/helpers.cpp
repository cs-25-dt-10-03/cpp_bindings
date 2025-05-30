#include "../include/helpers.h"
#include "../include/flexoffer.h"
#include "../include/config.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>
#include <cmath>

using namespace std;

tuple<int, int> compute_aggregated_window(const vector<Flexoffer>& flex_offers) {
    time_t global_earliest = numeric_limits<time_t>::max();

    // First pass: find minimum earliest start
    for (const auto& fo : flex_offers) {
        global_earliest = min(global_earliest, fo.get_est());
    }

    // Second pass: find min(lst_i - (est_i - global_earliest))
    time_t min_time_flex = std::numeric_limits<time_t>::max();
    for (const auto& fo : flex_offers) {
        time_t time_flex = fo.get_lst() - fo.get_est();
        min_time_flex = std::min(min_time_flex, time_flex);
    }
    time_t aggregated_latest = global_earliest + min_time_flex;


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

Flexoffer aggregate_two(const Flexoffer& a,
                        const Flexoffer& b,
                        int offset){

    // 1) Compute how far left we need to go
    int start_shift = std::min(0, offset);

    // 2) end‐point is furthest slot we need: max(A_end, B_end)
    int end_point = std::max(a.get_duration(), offset + b.get_duration());
    int new_length  = end_point - start_shift;  


    // 2) Initialize aggregated profile to zero
    std::vector<TimeSlice> new_profile(new_length, TimeSlice(0.0, 0.0));

    const auto& profA = a.get_profile();
    const auto& profB = b.get_profile();


    // lay down A at [-start_shift .. -start_shift + a.duration-1]
    for (int i = 0; i < a.get_duration(); ++i) {
        int pos = i - start_shift;
        new_profile[pos].min_power += profA[i].min_power;
        new_profile[pos].max_power += profA[i].max_power;
    }

    // lay down B at [offset-start_shift .. offset-start_shift + b.duration-1]
    for (int i = 0; i < b.get_duration(); ++i) {
        int pos = offset + i - start_shift;
        if (pos >= 0 && pos < new_length) {
            new_profile[pos].min_power += profB[i].min_power;
            new_profile[pos].max_power += profB[i].max_power;
        }
    }
            
    
    // 5) Compute the merged flex‐start window:
    //    A can start in [a.est, a.lst]
    //    B (shifted) can start at T such that
    //       (T + offset*RES) ∈ [b.est, b.lst]
    //    ⇒ T ∈ [b.est - offset*RES,  b.lst - offset*RES]
    const time_t off_sec = offset * TIME_RESOLUTION;
    time_t new_est = std::max(a.get_est(),
                              b.get_est() - off_sec);
    time_t new_lst = std::min(a.get_lst(),
                              b.get_lst() - off_sec);

    // 6) Latest‐end tracks new_lst + duration
    time_t new_et  = new_lst + new_length * TIME_RESOLUTION;

    // 7) Sum the overall‐allocation bounds
    double new_min_alloc = a.get_min_overall_alloc()
                         + b.get_min_overall_alloc();
    double new_max_alloc = a.get_max_overall_alloc()
                         + b.get_max_overall_alloc();

    // 8) Return the aggregated, still‐flexible Flexoffer
    //    id = -1 is a placeholder
    return Flexoffer(
        -1,
        new_est,      // earliest_start
        new_lst,      // latest_start  (nonzero slack)
        new_et,       // end_time = latest_start + duration
        new_profile,  // merged profile
        new_length,   // duration in slots
        new_min_alloc,
        new_max_alloc
    );
}