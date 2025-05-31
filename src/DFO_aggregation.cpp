#include "../include/DFO_aggregation.h"
#include "../include/config.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cmath>

using namespace std;

/** 🔹 Helper function: Generates start padding polygons with zero dependency and zero usage. */
vector<DependencyPolygon> DFO_Aggregation::createStartPadding(int num_padding, int numsamples) {
    vector<DependencyPolygon> start_polygons;

    for (int i = 0; i < num_padding; i++) {
        DependencyPolygon polygon(0.0, 0.0, numsamples);
        polygon.charging_power = 0.0; // Set charging power to zero for padding polygons
        polygon.min_prev_energy = 0.0; // Set min_prev_energy to zero
        polygon.max_prev_energy = 0.0; // Set max_prev_energy to zero
        polygon.points.emplace_back(0.0, 0.0);
        polygon.points.emplace_back(0.0, 0.0);  // Two (0,0) points
        start_polygons.push_back(polygon);
    }

    return start_polygons;
}

/** 🔹 Helper function: Generates end padding polygons based on the last real polygon's total energy constraints. */
vector<DependencyPolygon> DFO_Aggregation::createEndPadding(const DFO& dfo, int num_padding, int numsamples) {
    vector<DependencyPolygon> end_polygons;

    if (dfo.polygons.empty() || num_padding <= 0) return end_polygons;

    const DependencyPolygon& last_polygon = dfo.polygons.back();

    double min_total_energy = numeric_limits<double>::max(); // Set to highest value possible
    double max_total_energy = numeric_limits<double>::lowest(); // Set to lowest value possible

    for (const Point& p : last_polygon.points) { // Loop through all points to find the lowest and highest total energy
        double total_energy = p.x + p.y;
        min_total_energy = min(min_total_energy, total_energy);
        max_total_energy = max(max_total_energy, total_energy);
    }

    for (int i = 0; i < num_padding; i++) {
        DependencyPolygon polygon(min_total_energy, max_total_energy, numsamples);
        polygon.charging_power = 0.0; // Set charging power to zero for padding polygons
        polygon.min_prev_energy = min_total_energy; // Set min_prev_energy to min_total_energy
        polygon.max_prev_energy = max_total_energy; // Set max_prev_energy to max_total_energy
        // Four points (min_total_energy, 0) and (max_total_energy, 0) to create a rectangle representing the polygon
        polygon.points.emplace_back(min_total_energy, 0.0);
        polygon.points.emplace_back(min_total_energy, 0.0);
        polygon.points.emplace_back(max_total_energy, 0.0);
        polygon.points.emplace_back(max_total_energy, 0.0);
        end_polygons.push_back(polygon);
    }

    return end_polygons;
}

tuple<vector<DFO>, int> DFO_Aggregation::padDFOsToCommonTimeline(const vector<DFO>& dfos) {
    if (dfos.empty()) {
        vector<DFO> res = {};
        return { res, 0 };
    }

    // 🔹 Find earliest start time
    time_t earliest_start = dfos[0].get_est();
    for (const DFO& dfo : dfos) {
        earliest_start = min(earliest_start, dfo.get_est());
    }

    // 🔹 Calculate how much padding each DFO needs
    vector<int> start_paddings;
    vector<int> end_paddings;
    vector<int> lengths;

    for (const DFO& dfo : dfos) {
        int pad_start = static_cast<int>((dfo.get_est() - earliest_start) / TIME_RESOLUTION);
        start_paddings.push_back(pad_start);
        lengths.push_back(pad_start + dfo.polygons.size());
    }

    int common_timeline_length = *max_element(lengths.begin(), lengths.end());

    for (size_t i = 0; i < dfos.size(); i++) {
        int pad_end = common_timeline_length - (start_paddings[i] + dfos[i].polygons.size());
        end_paddings.push_back(pad_end);
    }

    // 🔹 Create padded DFOs
    vector<DFO> padded_dfos;
    for (size_t i = 0; i < dfos.size(); i++) {
        const DFO& dfo = dfos[i];
        int numsamples = dfo.polygons[0].numsamples; // Get the number of samples from the first polygon
        DFO padded = dfo; // Copy original

        // Clear polygons and build new ones
        padded.polygons.clear();
        
        vector<DependencyPolygon> start_padding = createStartPadding(start_paddings[i], numsamples);
        vector<DependencyPolygon> end_padding = createEndPadding(dfo, end_paddings[i], numsamples);

        // Concatenate: start padding + original polygons + end padding
        padded.polygons.insert(padded.polygons.end(), start_padding.begin(), start_padding.end());
        padded.polygons.insert(padded.polygons.end(), dfo.polygons.begin(), dfo.polygons.end());
        padded.polygons.insert(padded.polygons.end(), end_padding.begin(), end_padding.end());

        // Adjust the earliest_start to match the new timeline
        padded.earliest_start_time = earliest_start;

        padded_dfos.push_back(padded);
    }

    return { padded_dfos, common_timeline_length };
}

/** 🔹 Helper function: Performs linear interpolation */
double DFO_Aggregation::linearInterpolation(double x, double x0, double y0, double x1, double y1) {
    if (x1 == x0) return (y0 + y1) / 2.0;  // Prevent division by zero
    return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/** 🔹 Helper function: Finds or interpolates points for a given dependency value. */
vector<Point> DFO_Aggregation::findOrInterpolatePoints(const vector<Point>& points, double dependency_value) {
    const double EPSILON = 1e-9; // Small epsilon for floating point comparison

    // 1. 🧠 Handle empty points vector (should not happen with proper padding)
    if (points.empty()) {
        // Fallback for an empty polygon, return zero usage for the given dependency
        return {Point(dependency_value, 0.0), Point(dependency_value, 0.0)};
    }

    // 2. 🧠 Try to find exact matches for dependency_value (x-coordinate)
    vector<Point> exact_matches;
    for (const Point& p : points) {
        if (abs(p.x - dependency_value) < EPSILON) {
            exact_matches.push_back(p);
        }
    }

    if (!exact_matches.empty()) {
        // 🧠🧠🧠
        // If exact matches are found, determine the min and max y among them.
        // Since points are sorted by x then y, if multiple points have the same x,
        // the first will be the min y, and the last will be the max y for that x.
        // If there's only one point for an x (meaning min_y == max_y), return it twice.
        if (exact_matches.size() == 1) {
            return {exact_matches[0], exact_matches[0]};
        } else {
            // Find the true min and max y among the matching points
            double min_y = numeric_limits<double>::max();
            double max_y = numeric_limits<double>::lowest();
            for (const auto& p : exact_matches) {
                min_y = min(min_y, p.y);
                max_y = max(max_y, p.y);
            }
            return {Point(dependency_value, min_y), Point(dependency_value, max_y)};
        }
    }

    // 3. If no exact match, perform linear interpolation.🧠🧠
    // Ensure dependency_value is within the x-range of the polygon's points
    if (dependency_value < points.front().x) {
        // Clamp to the first pair of points
        // Assuming points[0] is (x_min, y_min) and points[1] is (x_min, y_max)
        return {points[0], points[1]};
    }
    if (dependency_value > points.back().x) {
        // Clamp to the last pair of points
        // Assuming points.at(size-2) is (x_max, y_min) and points.back() is (x_max, y_max)
        return {points.at(points.size() - 2), points.back()};
    }

    // Now, dependency_value is strictly between distinct x-values, or exactly on one.
    // If we're here, it means `dependency_value` doesn't exactly match any `p.x`,
    // but it's within the overall `x` range.🧠
    // We need to find the `x_i` and `x_{i+1}` that bracket `dependency_value`.🧠
    // The points are structured as `(x_0, y_0_min), (x_0, y_0_max), (x_1, y_1_min), (x_1, y_1_max), ...`
    // So we're looking for an index `idx` such that `points[idx].x` and `points[idx+1].x` correspond
    // to `x_i`, and `points[idx+2].x` and `points[idx+3].x` correspond to `x_{i+1}`.🧠🧠🧠

    size_t idx_current_x_min_y = 0;
    // Iterate through the pairs of points to find the lower bound for interpolation
    // 🧠We need at least 4 points for an interpolation segment (2 for start X, 2 for end X)
    while (idx_current_x_min_y + 3 < points.size() && // Ensure we have a next pair for interpolation
           points[idx_current_x_min_y + 2].x < dependency_value) // Check next distinct x value
    {
        idx_current_x_min_y += 2; // Move to the start of the next (x,y_min),(x,y_max) pair
    }

    // Define the bounding points for interpolation
    const Point& prev_min_point = points[idx_current_x_min_y];       // 🧠(x_i, y_i_min)
    const Point& prev_max_point = points[idx_current_x_min_y + 1];   // 🧠(x_i, y_i_max)
    const Point& next_min_point = points[idx_current_x_min_y + 2];   // 🧠(x_{i+1}, y_{i+1}_min)
    const Point& next_max_point = points[idx_current_x_min_y + 3];   // 🧠(x_{i+1}, y_{i+1}_max)

    // Calculate interpolated minimum energy (s_min)
    double s_min = linearInterpolation(dependency_value, prev_min_point.x, prev_min_point.y, next_min_point.x, next_min_point.y);

    // 🧠🧠🧠
    // For cases where the previous slope of interpolation would reach 0 before the next points,
    // such that it did reach a higher value than simply interpolating between previous points.
    // This translates to a specific rule: if the future minimum required energy for the next X-value is zero,
    // then the current minimum energy usage might be constrained by the total energy needed.
    if (abs(next_min_point.y - 0.0) < EPSILON) { // Check if the next point's minimum y is (effectively) zero
        // The original logic was: max((prev_min.x + prev_min.y) - dependency_value, 0.0)
        // This calculates the remaining energy needed from the perspective of the *previous* min point.
        // It's effectively saying: if you used (prev_min.x + prev_min.y) by the end of this current timestep,
        // and you're at `dependency_value` now, how much more do you *need* to meet that minimum, capped at 0?
        // 🧠🧠
        s_min = max((prev_min_point.x + prev_min_point.y) - dependency_value, 0.0);
    }

    // 🧠 Calculate interpolated maximum energy (s_max)
    double s_max = linearInterpolation(dependency_value, prev_max_point.x, prev_max_point.y, next_max_point.x, next_max_point.y);

    // 🧠Ensure s_min <= s_max due to potential floating point inaccuracies
    if (s_min > s_max) {
        swap(s_min, s_max);
    }

    return {Point(dependency_value, s_min), Point(dependency_value, s_max)};
}

/** 🔹 Function: Aggregates two DFOs into one, handling misaligned start times by padding with temporary polygons. */
DFO DFO_Aggregation::agg2to1(const DFO& dfo1, const DFO& dfo2, int numsamples) {

    // Determine the earliest start time
    time_t start_time = min(dfo1.earliest_start_time, dfo2.earliest_start_time);

    // Compute how many padding polygons are needed at the start for each DFO
    int pad_start_1 = static_cast<int>((dfo1.earliest_start_time - start_time) / TIME_RESOLUTION);
    int pad_start_2 = static_cast<int>((dfo2.earliest_start_time - start_time) / TIME_RESOLUTION);

    // Compute how many padding polygons are needed at the end for each DFO
    int max_length = max(dfo1.polygons.size() + pad_start_1, dfo2.polygons.size() + pad_start_2);
    int pad_end_1 = max_length - (dfo1.polygons.size() + pad_start_1);
    int pad_end_2 = max_length - (dfo2.polygons.size() + pad_start_2);

    // Create padded versions of the polygons
    vector<DependencyPolygon> padded_polygons_1 = createStartPadding(pad_start_1, numsamples);
    padded_polygons_1.insert(padded_polygons_1.end(), dfo1.polygons.begin(), dfo1.polygons.end());
    vector<DependencyPolygon> end_padding_1 = createEndPadding(dfo1, pad_end_1, numsamples);
    padded_polygons_1.insert(padded_polygons_1.end(), end_padding_1.begin(), end_padding_1.end());

    vector<DependencyPolygon> padded_polygons_2 = createStartPadding(pad_start_2, numsamples);
    padded_polygons_2.insert(padded_polygons_2.end(), dfo2.polygons.begin(), dfo2.polygons.end());
    vector<DependencyPolygon> end_padding_2 = createEndPadding(dfo2, pad_end_2, numsamples);
    padded_polygons_2.insert(padded_polygons_2.end(), end_padding_2.begin(), end_padding_2.end());

    // Aggregate the aligned polygons
    vector<DependencyPolygon> aggregated_polygons;


    for (int i = 0; i < max_length; i++) {
        const DependencyPolygon& polygon1 = padded_polygons_1[i];
        const DependencyPolygon& polygon2 = padded_polygons_2[i];

        double aggregated_min_prev = polygon1.min_prev_energy + polygon2.min_prev_energy;
        double aggregated_max_prev = polygon1.max_prev_energy + polygon2.max_prev_energy;
        double aggregated_charging_power = polygon1.charging_power + polygon2.charging_power;

        DependencyPolygon aggregated_polygon(aggregated_min_prev, aggregated_max_prev, numsamples);
        aggregated_polygon.charging_power = aggregated_charging_power;
        aggregated_polygon.min_prev_energy = aggregated_min_prev;
        aggregated_polygon.max_prev_energy = aggregated_max_prev;

        if (polygon1.points.size() == 2 && polygon2.points.size() == 2) {
            // Special case: only two points (e.g., first timestep with min/max at 0)
            double min_current_energy = polygon1.points[0].y + polygon2.points[0].y;
            double max_current_energy = polygon1.points[1].y + polygon2.points[1].y;
            double dependency_amount = polygon1.points[1].x + polygon2.points[1].x;

            aggregated_polygon.add_point(dependency_amount, min_current_energy);
            aggregated_polygon.add_point(dependency_amount, max_current_energy);

        } else {
            // General case: Iterate from min dependency to max dependency
            double step1 = (polygon1.max_prev_energy - polygon1.min_prev_energy) / (numsamples - 1);
            double step2 = (polygon2.max_prev_energy - polygon2.min_prev_energy) / (numsamples - 1);
            double step = (aggregated_max_prev - aggregated_min_prev) / (numsamples - 1);

            for (int j = 0; j < numsamples; j++) {
                double current_prev_energy1 = polygon1.min_prev_energy + j * step1;
                double current_prev_energy2 = polygon2.min_prev_energy + j * step2;
                double current_prev_energy = aggregated_min_prev + j * step;

                // Find or interpolate min/max energy usage for DFO1
                auto matching_points1 = findOrInterpolatePoints(polygon1.points, current_prev_energy1);
                double dfo1_min_energy = matching_points1[0].y;
                double dfo1_max_energy = matching_points1[1].y;

                // Find or interpolate min/max energy usage for DFO2
                auto matching_points2 = findOrInterpolatePoints(polygon2.points, current_prev_energy2);
                double dfo2_min_energy = matching_points2[0].y;
                double dfo2_max_energy = matching_points2[1].y;

                // Aggregate min/max energy
                double min_current_energy = dfo1_min_energy + dfo2_min_energy;
                double max_current_energy = dfo1_max_energy + dfo2_max_energy;

                aggregated_polygon.add_point(current_prev_energy, min_current_energy);
                aggregated_polygon.add_point(current_prev_energy, max_current_energy);
            }
        }

        aggregated_polygons.push_back(aggregated_polygon);
    }

    double min_overall_energy = dfo1.min_total_energy + dfo2.min_total_energy;
    double max_overall_energy = dfo1.max_total_energy + dfo2.max_total_energy;


    DFO aggregated_DFO = DFO(-1, {0}, {0}, numsamples, 0.0, min_overall_energy, max_overall_energy, start_time);

    aggregated_DFO.polygons = aggregated_polygons;
    aggregated_DFO.calculate_latest_start_time();
    return aggregated_DFO;
}

/** 🔹 Aggregates multiple DFOs into one using accumulating pairwise aggregation */
DFO DFO_Aggregation::aggnto1(const vector<DFO>& dfos, int numsamples) {
    if (dfos.empty()) {
        throw runtime_error("No DFOs provided for aggregation. Kind Regards, aggnto1 function");
    }

    // Start aggregation with the first DFO
    DFO aggregated_dfo = dfos[0];

    // Aggregate subsequent DFOs
    for (size_t i = 1; i < dfos.size(); i++) {
        aggregated_dfo = agg2to1(aggregated_dfo, dfos[i], numsamples);
    }

    return aggregated_dfo;
}
