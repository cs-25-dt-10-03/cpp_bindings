#include "../include/config.h"
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
    double timestep_scaling = TIME_RESOLUTION / 3600.0;
    double adjusted_charging_power = charging_power * timestep_scaling;

    if (min_prev_energy == max_prev_energy) {
        double min_current_energy = max(next_min_prev - min_prev_energy, 0.0);
        min_current_energy = min(min_current_energy, adjusted_charging_power); // Limit to charging power
        double max_current_energy = max(next_max_prev - min_prev_energy, 0.0);
        max_current_energy = min(max_current_energy, adjusted_charging_power); // Limit to charging power

        add_point(min_prev_energy, min_current_energy);
        add_point(max_prev_energy, max_current_energy);
        return;
    }

    double step = (max_prev_energy - min_prev_energy) / (numsamples - 1);
    for (int i = 0; i < numsamples; ++i) {
        double current_prev_energy = min_prev_energy + i * step;

        // Calculate the min and max energy needed for the next time slice
        double min_current_energy = max(next_min_prev - current_prev_energy, 0.0);
        min_current_energy = min(min_current_energy, adjusted_charging_power); // Limit to charging power
        double max_current_energy = max(next_max_prev - current_prev_energy, 0.0);
        max_current_energy = min(max_current_energy, adjusted_charging_power); // Limit to charging power

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
         int numsamples, double charging_power, double min_total_energy, double max_total_energy, time_t earliest_start_time)
    : dfo_id(dfo_id), charging_power(charging_power), earliest_start_time(earliest_start_time), latest_start_time(earliest_start_time) {

        
    if (min_prev.empty() || max_prev.empty()) {
        throw runtime_error("min_prev and max_prev cannot be empty.");
    }
    
    for (size_t i = 0; i < min_prev.size(); ++i) {
        polygons.emplace_back(min_prev[i], max_prev[i], numsamples);
    }


    this->end_time = this->earliest_start_time + static_cast<time_t>(polygons.size()) * TIME_RESOLUTION;

    this->min_total_energy = (min_total_energy == -1) ? min_prev.back() : min_total_energy;
    this->max_total_energy = (max_total_energy == -1) ? max_prev.back() : max_total_energy;

    if (charging_power != 0.0) {
        double required_charging_time = min_total_energy / charging_power;
        int required_time_slots = ceil(required_charging_time * (3600 / TIME_RESOLUTION));
        this->latest_start_time = this->end_time - required_time_slots * TIME_RESOLUTION;
    }
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

void DFO::calculate_latest_start_time() {
    latest_start_time = earliest_start_time;
    for (const auto& polygon : polygons) {
        if (polygon.points.front().y == 0) {
            latest_start_time += TIME_RESOLUTION;
        } else {
            break;
        }
    }
}

// Additional methods
int DFO::get_est_hour() const {
    struct tm* timeinfo = localtime(&earliest_start_time);
    return timeinfo->tm_hour;
}

int DFO::get_lst_hour() const {
    struct tm* timeinfo = localtime(&latest_start_time);
    return timeinfo->tm_hour;
}

int DFO::get_et_hour() const {
    struct tm* timeinfo = localtime(&end_time);
    return timeinfo->tm_hour;
}

time_t DFO::get_est() const {return earliest_start_time;};
time_t DFO::get_lst() const {return latest_start_time;};
time_t DFO::get_et() const {return end_time;}