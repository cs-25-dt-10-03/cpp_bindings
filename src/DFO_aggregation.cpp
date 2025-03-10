#include "../include/DFO_aggregation.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace std;

/** 🔹 Helper function: Generates start padding polygons with zero dependency and zero usage. */
vector<DependencyPolygon> DFO_Aggregation::createStartPadding(int num_padding, int numsamples) {
    vector<DependencyPolygon> start_polygons;

    for (int i = 0; i < num_padding; i++) {
        DependencyPolygon polygon(0.0, 0.0, numsamples);
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
        // Four points (min_total_energy, 0) and (max_total_energy, 0) to create a rectangle representing the polygon
        polygon.points.emplace_back(min_total_energy, 0.0);
        polygon.points.emplace_back(min_total_energy, 0.0);
        polygon.points.emplace_back(max_total_energy, 0.0);
        polygon.points.emplace_back(max_total_energy, 0.0);
        end_polygons.push_back(polygon);
    }

    return end_polygons;
}

/** 🔹 Helper function: Performs linear interpolation */
double DFO_Aggregation::linearInterpolation(double x, double x0, double y0, double x1, double y1) {
    if (x1 == x0) return (y0 + y1) / 2.0;  // Prevent division by zero
    return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/** 🔹 Helper function: Finds or interpolates points for a given dependency value. */
vector<Point> DFO_Aggregation::findOrInterpolatePoints(const vector<Point>& points, double dependency_value) {
    vector<Point> matching_points;
    
    for (const Point& p : points) {
        if (p.x == dependency_value) { // try find existing points with that dependency value
            matching_points.push_back(p);
        }
    }

    if (!matching_points.empty()) return matching_points;

    // Otherwise perform linear interpolation on points before and after dependency_value
    for (size_t k = 1; k + 1 < points.size(); k += 2) {
        const Point& prev_min = points[k - 1];
        const Point& prev_max = points[k];
        const Point& next_min = points[k + 1];
        const Point& next_max = points[k + 2];

        if (dependency_value >= prev_min.x && dependency_value <= next_min.x) {
            double s_min = linearInterpolation(dependency_value, prev_min.x, prev_min.y, next_min.x, next_min.y);
            double s_max = linearInterpolation(dependency_value, prev_max.x, prev_max.y, next_max.x, next_max.y);
            return { Point(dependency_value, s_min), Point(dependency_value, s_max) };
        }
    }

    return matching_points;
}

/** 🔹 Function: Aggregates two DFOs into one, handling misaligned start times by padding with temporary polygons. */
DFO DFO_Aggregation::agg2to1(const DFO& dfo1, const DFO& dfo2, int numsamples) {

    // Determine the earliest start time
    time_t start_time = min(dfo1.earliest_start, dfo2.earliest_start);

    // Compute how many padding polygons are needed at the start for each DFO
    int pad_start_1 = static_cast<int>((dfo1.earliest_start - start_time) / 3600);
    int pad_start_2 = static_cast<int>((dfo2.earliest_start - start_time) / 3600);

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

        DependencyPolygon aggregated_polygon(aggregated_min_prev, aggregated_max_prev, numsamples);

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

    DFO aggregated_DFO = DFO(-1, {0}, {0}, numsamples, 0.0, -1, -1, start_time);
    aggregated_DFO.polygons = aggregated_polygons;
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
