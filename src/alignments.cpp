#include "../include/alignments.h"
#include "../include/flexoffer.h"
#include "../include/config.h"
#include "../include/helpers.h"

#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>

Flexoffer start_alignment_aggregate(const vector<Flexoffer>& flex_offers) {

    time_t global_earliest, aggregated_latest;
    tie(global_earliest, aggregated_latest) = compute_aggregated_window(flex_offers);

    int common_length;
    vector<int> offsets = compute_offsets_and_length(flex_offers, global_earliest, common_length);

    vector<TimeSlice> aggregated_profile(common_length, TimeSlice(0.0, 0.0));

    double agg_total_max = 0;
    double agg_total_min = 0;

    for (size_t i = 0; i < flex_offers.size(); i++) {
        int offset = offsets[i];
        const auto& profile = flex_offers[i].get_profile();

        for (size_t j = 0; j < profile.size(); j++) {
            int index = offset + j;
            aggregated_profile[index].min_power += profile[j].min_power;
            aggregated_profile[index].max_power += profile[j].max_power;
        }
        
        agg_total_min += flex_offers[i].get_min_overall_alloc();
        agg_total_max += flex_offers[i].get_max_overall_alloc();
    }

    return Flexoffer(
        -1,
        global_earliest,
        aggregated_latest,
        aggregated_latest,
        aggregated_profile,
        common_length,
        agg_total_min,
        agg_total_max
    );
}