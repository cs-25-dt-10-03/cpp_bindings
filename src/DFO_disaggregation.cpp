#include "DFO_disaggregation.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

/*
Disaggregates a single aggregated DFO (DA) into two original DFOs (D1 and D2), accounting for start time offsets.
    
    Args:
        D1 (DFO): First original DFO.
        D2 (DFO): Second original DFO.
        DA (DFO): Aggregated DFO.
        yA_ref (vector<double>&): Reference schedule for the aggregated DFO.
    
    Returns:
        y1_ref (vector<double>): Disaggregated schedule for DFO 1.
        y2_ref (vector<double>): Disaggregated schedule for DFO 2.
*/
pair<vector<double>, vector<double>> DFO_disaggregation::disagg1to2(
    const DFO& D1, const DFO& D2, const DFO& DA, const vector<double>& yA_ref) {
    
    size_t T = DA.polygons.size();
    if (T != yA_ref.size()) {
        throw runtime_error("Mismatch between DA timesteps and yA_ref size. Kind regards, disagg1to2 function");
    }

    // Determine offsets in start time
    time_t start_time = min(D1.earliest_start, D2.earliest_start);
    int offset_1 = (D1.earliest_start - start_time) / 3600;
    int offset_2 = (D2.earliest_start - start_time) / 3600;

    // Determine overlapping region
    int overlap_start = max(offset_1, offset_2);
    int overlap_end = min(static_cast<int>(D1.polygons.size() + offset_1),
                               static_cast<int>(D2.polygons.size() + offset_2), static_cast<int>(T));

    // Initialize output vectors
    vector<double> y1_ref(D1.polygons.size(), 0.0);
    vector<double> y2_ref(D2.polygons.size(), 0.0);

    double dA = 0.0, d1 = 0.0, d2 = 0.0;

    // Assign reference values directly for non-overlapping start
    for (int i = 0; i < offset_2; i++) {
        y1_ref[i] = yA_ref[i]; // Assign entire aggregated schedule to DFO1 if it starts earlier
        d1 += yA_ref[i];
        dA += yA_ref[i];
    }
    for (int i = 0; i < offset_1; i++) {
        y2_ref[i] = yA_ref[i]; // Assign entire aggregated schedule to DFO2 if it starts earlier
        d2 += yA_ref[i];
        dA += yA_ref[i];
    }

    // Handle overlapping section using normal interpolation logic
    for (int i = overlap_start; i < overlap_end; i++) {
        const DependencyPolygon& polygonA = DA.polygons[i];
        const DependencyPolygon& polygon1 = D1.polygons[i - offset_1];
        const DependencyPolygon& polygon2 = D2.polygons[i - offset_2];

        // Find points with the respective energy dependency for DFO A, DFO 1, and DFO 2
        auto matching_pointsA = DFO_Aggregation::findOrInterpolatePoints(polygonA.points, dA);
        auto matching_points1 = DFO_Aggregation::findOrInterpolatePoints(polygon1.points, d1);
        auto matching_points2 = DFO_Aggregation::findOrInterpolatePoints(polygon2.points, d2);

        // Calculate scaling factor f based on the reference schedule
        Point pointA1 = matching_pointsA[0], pointA2 = matching_pointsA[1];
        double f = (pointA2.y - pointA1.y == 0) ? 0 : (yA_ref[i] - pointA1.y) / (pointA2.y - pointA1.y);

        // Use scaling factor on DFO 1 and 2 to determine their energy usage
        Point point1_1 = matching_points1[0], point1_2 = matching_points1[1];
        y1_ref[i - offset_1] = point1_1.y + f * (point1_2.y - point1_1.y);

        Point point2_1 = matching_points2[0], point2_2 = matching_points2[1];
        y2_ref[i - offset_2] = point2_1.y + f * (point2_2.y - point2_1.y);

        // Update cumulative dependency amounts for DFO 1, DFO 2, and DFO A
        dA += yA_ref[i];
        d1 += y1_ref[i - offset_1];
        d2 += y2_ref[i - offset_2];
    }

    // Assign reference values directly for non-overlapping end
    for (size_t i = overlap_end; i < T; i++) {
        if (i >= D1.polygons.size() + offset_1) {
            y2_ref[i - offset_2] = yA_ref[i];  // Assign remaining schedule to DFO2 if it extends longer
        }
        if (i >= D2.polygons.size() + offset_2) {
            y1_ref[i - offset_1] = yA_ref[i];  // Assign remaining schedule to DFO1 if it extends longer
        }
    }

    return {y1_ref, y2_ref};
}

/*
Disaggregates a single aggregated DFO (DA) into multiple original DFOs (DFOs), 
    ensuring that only the DFOs active in a given hour participate in the disaggregation.

    Args:
        DA (DFO): Aggregated DFO.
        DFOs (vector<DFO>&): List of original DFOs.
        yA_ref (vector<double>&): Reference schedule for the aggregated DFO.

    Returns:
        y_refs (vector<vector<double>>): Disaggregated schedules for each DFO.
*/
vector<vector<double>> DFO_disaggregation::disagg1toN(
    const DFO& DA, const vector<DFO>& DFOs, const vector<double>& yA_ref) {
    
    size_t T = DA.polygons.size(); // Number of timesteps
    size_t N = DFOs.size(); // Number of DFOs
    
    if (T != yA_ref.size()) {
        throw runtime_error("Mismatch between DA timesteps and yA_ref size. Kind regards, disagg1toN function");
    }

    // Find the earliest start time across all DFOs
    time_t start_time = DFOs[0].earliest_start;
    for (const auto& dfo : DFOs) {
        start_time = min(start_time, dfo.earliest_start);
    }

    // Calculate offsets and end times for each DFO
    vector<int> offsets(N);
    vector<int> end_times(N);
    for (size_t i = 0; i < N; i++) {
        offsets[i] = (DFOs[i].earliest_start - start_time) / 3600;
        end_times[i] = offsets[i] + DFOs[i].polygons.size();
    }

    // Initialize output lists
    vector<vector<double>> y_refs(N, vector<double>(T, 0.0));

    // Initialize dependency tracking
    vector<double> d(N, 0.0); // Dependency amounts for each DFO
    double dA = 0.0;

    // Iterate over each timestep in the aggregated DFO
    for (size_t i = 0; i < T; i++) {
        // Get the polygon slice for the timestep
        const DependencyPolygon& polygonA = DA.polygons[i];

        // Determine which DFOs are active at this timestep
        vector<int> active_dfo_indices;
        for (size_t j = 0; j < N; j++) {
            if (offsets[j] <= i && i < end_times[j]) {
                active_dfo_indices.push_back(j);
            }
        }

        // If no DFOs are active, skip this timestep
        if (active_dfo_indices.empty()) continue;

        // Find points with the respective energy dependency for aggregated DFO A
        auto matching_pointsA = DFO_Aggregation::findOrInterpolatePoints(polygonA.points, dA);
        Point pointA1 = matching_pointsA[0], pointA2 = matching_pointsA[1];

        // Calculate scaling factor f based on the reference schedule
        double f = (pointA2.y - pointA1.y == 0) ? 0 : (yA_ref[i] - pointA1.y) / (pointA2.y - pointA1.y);

        // Disaggregate among active DFOs
        for (int j : active_dfo_indices) {
            const DependencyPolygon& polygon = DFOs[j].polygons[i - offsets[j]];
            auto matching_points = DFO_Aggregation::findOrInterpolatePoints(polygon.points, d[j]);
            Point point1 = matching_points[0], point2 = matching_points[1];

            // Update reference schedule and dependency amount for the current DFO
            y_refs[j][i - offsets[j]] = point1.y + f * (point2.y - point1.y);
            d[j] += y_refs[j][i - offsets[j]];
        }

        // Update dependency amount for the aggregated DFO
        dA += yA_ref[i];
    }

    return y_refs;
}