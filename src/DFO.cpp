#include "../include/DFO.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>
#include <algorithm>

using namespace std;

Point::Point(double x_val, double y_val) : x(x_val), y(y_val) {}

string Point::to_string() const {
    ostringstream oss;
    oss << "(" << fixed << setprecision(3) << x << ", " << y << ")"; // 3 Significant digits
    return oss.str();
}

// Overloaded << operator for Point for easy printing
ostream& operator<<(ostream& os, const Point& point) {
    os << point.to_string();
    return os;
}

DependencyPolygon::DependencyPolygon(double min_prev, double max_prev, int numsamples)
    : min_prev_energy(min_prev), max_prev_energy(max_prev), numsamples(numsamples) {}

void DependencyPolygon::generate_polygon(double charging_power, double next_min_prev, double next_max_prev) {
    if (min_prev_energy == max_prev_energy) {
        double min_current_energy = max(next_min_prev - min_prev_energy, 0.0);
        min_current_energy = min(min_current_energy, charging_power); // Limit to charging power
        double max_current_energy = max(next_max_prev - min_prev_energy, 0.0);
        max_current_energy = min(max_current_energy, charging_power); // Limit to charging power

        add_point(min_prev_energy, min_current_energy);
        add_point(max_prev_energy, max_current_energy);
        return;
    }

    double step = (max_prev_energy - min_prev_energy) / (numsamples - 1);
    for (int i = 0; i < numsamples; ++i) {
        double current_prev_energy = min_prev_energy + i * step;

        // Calculate the min and max energy needed for the next time slice
        double min_current_energy = max(next_min_prev - current_prev_energy, 0.0);
        min_current_energy = min(min_current_energy, charging_power); // Limit to charging power
        double max_current_energy = max(next_max_prev - current_prev_energy, 0.0);
        max_current_energy = min(max_current_energy, charging_power); // Limit to charging power

        // Add the points to the polygon
        add_point(current_prev_energy, min_current_energy);
        add_point(current_prev_energy, max_current_energy);
    }

    sort_points();
}

void DependencyPolygon::add_point(double x, double y) {
    points.emplace_back(x, y);
}

void DependencyPolygon::sort_points() {
    sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
}

void DependencyPolygon::print_polygon(int index) const {
    cout << "Polygon " << index << ":\n";
    for (const auto& point : points) {
        cout << "  " << point.to_string() << "\n";
    }
}

string DependencyPolygon::to_string() const {
    ostringstream oss;
    oss << "Polygon:\n";
    for (const auto& point : points) {
        oss << "  " << point.to_string() << "\n";
    }
    return oss.str();
}

// Overloaded << operator for DependencyPolygon for easy printing
ostream& operator<<(ostream& os, const DependencyPolygon& polygon) {
    os << polygon.to_string();
    return os;
}

DFO::DFO(int dfo_id, const vector<double>& min_prev, const vector<double>& max_prev, 
         int numsamples, double charging_power, double min_total_energy, double max_total_energy, time_t earliest_start)
    : dfo_id(dfo_id), charging_power(charging_power), earliest_start(earliest_start), latest_start(earliest_start) {

    if (min_prev.empty() || max_prev.empty()) {
        throw runtime_error("min_prev and max_prev cannot be empty.");
    }

    for (size_t i = 0; i < min_prev.size(); ++i) {
        polygons.emplace_back(min_prev[i], max_prev[i], numsamples);
    }

    this->min_total_energy = (min_total_energy == -1) ? min_prev.back() : min_total_energy;
    this->max_total_energy = (max_total_energy == -1) ? max_prev.back() : max_total_energy;
}

void DFO::generate_dependency_polygons() {
    for (size_t i = 0; i < polygons.size(); ++i) {
        if (i < polygons.size() - 1) { // Generate allowed energy usage based on min/max dependency from the next timestep
            polygons[i].generate_polygon(charging_power, polygons[i + 1].min_prev_energy, polygons[i + 1].max_prev_energy);
        }
    }
    polygons.pop_back(); // Remove the last polygon, as it was only there such that the loop could generate the second-to-last polygon
}

void DFO::print_dfo() const {
    cout << "DFO ID: " << dfo_id << "\n";
    for (size_t i = 0; i < polygons.size(); ++i) {
        polygons[i].print_polygon(i);
    }
}

string DFO::to_string() const {
    ostringstream oss;
    oss << "DFO ID: " << dfo_id << "\n";
    for (size_t i = 0; i < polygons.size(); ++i) {
        oss << "Polygon " << i << ":\n" << polygons[i] << "\n";
    }
    return oss.str();
}

// Overloaded << operator for DFO for easy printing
ostream& operator<<(ostream& os, const DFO& dfo) {
    os << dfo.to_string();
    return os;
}