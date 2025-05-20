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

Flexoffer aggregate_two(const Flexoffer& a, const Flexoffer& b, int offset) {
    const std::vector<TimeSlice>& a_profile = a.get_profile();
    const std::vector<TimeSlice>& b_profile = b.get_profile();
    int a_duration = a.get_duration();
    int b_duration = b.get_duration();

    int a_offset = std::max(0, offset);
    int b_offset = std::max(0, -offset);

    int total_slots = std::max(a_offset + a_duration, b_offset + b_duration);

    std::vector<double> min_power(total_slots, 0.0);
    std::vector<double> max_power(total_slots, 0.0);

    for (int i = 0; i < a_duration; ++i) {
        min_power[a_offset + i] += a_profile[i].min_power;
        max_power[a_offset + i] += a_profile[i].max_power;
    }

    for (int i = 0; i < b_duration; ++i) {
        min_power[b_offset + i] += b_profile[i].min_power;
        max_power[b_offset + i] += b_profile[i].max_power;
    }

    std::vector<TimeSlice> new_profile;
    new_profile.reserve(total_slots);

    for (int i = 0; i < total_slots; ++i) {
        TimeSlice ts;
        ts.min_power = min_power[i];
        ts.max_power = max_power[i];
        new_profile.push_back(ts);
    }

    time_t new_start = std::min(a.get_est(), b.get_est());
    time_t new_lst = std::max(a.get_lst(), b.get_lst());
    time_t new_et = std::max(a.get_et(), b.get_et());

    double total_min = a.get_min_overall_alloc() + b.get_min_overall_alloc();
    double total_max = a.get_max_overall_alloc() + b.get_max_overall_alloc();

    return Flexoffer(-1, new_start, new_lst, new_et, std::move(new_profile),
                     total_slots, total_min, total_max);
}