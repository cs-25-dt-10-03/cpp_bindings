#include "../include/alignments.h"
#include "../include/flexoffer.h"
#include "../include/config.h"
#include "../include/helpers.h"



#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>
#include <iostream>
#include <cmath>
#include <queue>



Flexoffer start_alignment_aggregate(const vector<Flexoffer>& flex_offers) {
    time_t global_earliest, aggregated_latest;
    tie(global_earliest, aggregated_latest) = compute_aggregated_window(flex_offers);

    int common_length;
    vector<int> offsets = compute_offsets_and_length(flex_offers, global_earliest, common_length);

    vector<TimeSlice> aggregated_profile(common_length, TimeSlice(0.0, 0.0));

    double agg_total_max = 0;
    double agg_total_min = 0;

    for (size_t i = 0; i < flex_offers.size(); i++) {
        // std::cout << "[StartAlignment] Aggregating FO " << (i + 1) << "/" << flex_offers.size() << "\n";
        int offset = offsets[i];
        const auto& profile = flex_offers[i].get_profile();

        for (size_t j = 0; j < profile.size(); j++) {
            int index = offset + j;
            aggregated_profile[index].min_power += profile[j].min_power;
            aggregated_profile[index].max_power += profile[j].max_power;
            // cout << "[DEBUGGING] index and power on aggregated profile:" << index << " , " << aggregated_profile[index].max_power << "\n" << "index on and time current profile to be aggregated: " << j << " , " << profile[j].max_power << "\n";   
        }

        agg_total_min += flex_offers[i].get_min_overall_alloc();
        agg_total_max += flex_offers[i].get_max_overall_alloc();
    }

    time_t aggregated_end = aggregated_latest + common_length * TIME_RESOLUTION;

    return Flexoffer(
        -1,
        global_earliest,
        aggregated_latest,
        aggregated_end,
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
        // std::cout << "[BalanceAlignment] Step " << (count + 1)
        //   << "/" << (n - 1)
        //   << " — Merging with FO index " << candidateIndex << "\n";

        // Determine allowed offset range (in time slots)
        int lowerOffset = max(0, static_cast<int>((cand.get_est() - agg.get_est()) / TIME_RESOLUTION));
        int upperOffset = static_cast<int>((cand.get_lst() - agg.get_est()) / TIME_RESOLUTION);

        // Generate candidate offsets (evenly spaced between lowerOffset and upperOffset)
        vector<int> candidate_offsets;

        for (int k = 0; k < num_candidates; k++) {
            double fraction = static_cast<double>(k) / (num_candidates - 1);
            int off = lowerOffset + static_cast<int>(round(fraction * (upperOffset - lowerOffset)));
            candidate_offsets.push_back(off);
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

// Distance function: absolute balance used for merge quality
inline double balance_cost(const Flexoffer& a, const Flexoffer& b, int offset) {
    Flexoffer merged = aggregate_two(a, b, offset);
    return abs_balance(merged);
}

// Find the best offset to merge two FlexOffers, minimizing balance
Flexoffer merge_best_offset(const Flexoffer& a, const Flexoffer& b, int num_candidates) {
    int lowerOffset = max(0, static_cast<int>((b.get_est() - a.get_est()) / TIME_RESOLUTION));
    int upperOffset = static_cast<int>((b.get_lst() - a.get_est()) / TIME_RESOLUTION);

    vector<int> offsets;
    if (num_candidates <= 1) {
        offsets.push_back(lowerOffset);
    } else {
        for (int k = 0; k < num_candidates; ++k) {
            double fraction = static_cast<double>(k) / (num_candidates - 1);
            int off = lowerOffset + static_cast<int>(round(fraction * (upperOffset - lowerOffset)));
            offsets.push_back(off);
        }
    }

    double bestCost = numeric_limits<double>::max();
    Flexoffer bestAgg = a;

    for (int off : offsets) {
        Flexoffer temp = aggregate_two(a, b, off);
        double cost = abs_balance(temp);
        if (cost < bestCost) {
            bestCost = cost;
            bestAgg = temp;
        }
    }
    return bestAgg;
}

// Tree-based balanced aggregation
Flexoffer balance_alignment_tree_merge(const vector<Flexoffer> flex_offers, int num_candidates = 5) {
    if (flex_offers.size() == 1) return flex_offers[0];

    // Wrap each FO in a node
    vector<FO_Node> nodes;
    for (auto& fo : flex_offers) {
        nodes.emplace_back(fo);
    }

    // Binary merge using a priority queue (min-heap based on total flexibility)
    auto cmp = [&](int i, int j) {
        return compute_total_flex(nodes[i].offer) > compute_total_flex(nodes[j].offer);
    };

    priority_queue<int, vector<int>, decltype(cmp)> pq(cmp);
    for (int i = 0; i < nodes.size(); ++i) pq.push(i);

    while (pq.size() > 1) {
        int i = pq.top(); pq.pop();
        int j = pq.top(); pq.pop();

        Flexoffer merged = merge_best_offset(nodes[i].offer, nodes[j].offer, num_candidates);
        int new_depth = std::max<int>(nodes[i].depth, nodes[j].depth) + 1;

        nodes.emplace_back(merged, i, j, new_depth);
        pq.push((int)nodes.size() - 1);
    }

    return nodes[pq.top()].offer;  // Final aggregate
}
