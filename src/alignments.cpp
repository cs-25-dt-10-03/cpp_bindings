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

Flexoffer balance_alignment_aggregate(const vector<Flexoffer>& flex_offers, int num_candidates = 5) {

    size_t n = flex_offers.size();
    vector<bool> used(n, false);

    int base_index = get_least_flexible_index(flex_offers, used);
    used[base_index] = true;
    Flexoffer agg = flex_offers[base_index];
    double currentAbsBal = abs_balance(agg);

    // Iteratively aggregate the next least flexible offer.
    for (size_t count = 1; count < n; count++) {
        int candidateIndex = get_least_flexible_index(flex_offers, used);
        if (candidateIndex == -1)
            break; // All offers aggregated.
        
        const Flexoffer& cand = flex_offers[candidateIndex];
        
        // Determine allowed offset range (in time slots)
        int lowerOffset = max(0, static_cast<int>((cand.get_est() - agg.get_est()) / TIME_RESOLUTION));
        int upperOffset = static_cast<int>((cand.get_lst() - agg.get_est()) / TIME_RESOLUTION);

        // Generate candidate offsets (evenly spaced between lowerOffset and upperOffset)
        vector<int> candidate_offsets;
        if (num_candidates <= 1) {
            candidate_offsets.push_back(lowerOffset);
        } else {
            for (int k = 0; k < num_candidates; k++) {
                double fraction = static_cast<double>(k) / (num_candidates - 1);
                int off = lowerOffset + static_cast<int>(round(fraction * (upperOffset - lowerOffset)));
                candidate_offsets.push_back(off);
            }
        }

        // Evaluate each candidate offset to find the one minimizing the absolute balance.
        double bestCandidateBalance = numeric_limits<double>::max();
        int bestOffset = lowerOffset;
        Flexoffer candidateAgg = agg; // temporary candidate aggregation
        for (int off : candidate_offsets) {
            Flexoffer tempAgg = aggregate_two(agg, cand, off);
            double tempBal = abs_balance(tempAgg);
            if (tempBal < bestCandidateBalance) {
                bestCandidateBalance = tempBal;
                bestOffset = off;
                candidateAgg = tempAgg;
            }
        }
        
        // Update the aggregate with the candidate's best merge.
        agg = candidateAgg;
        currentAbsBal = bestCandidateBalance;
        used[candidateIndex] = true;
    }
    
    return agg;
}