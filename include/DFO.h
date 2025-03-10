#ifndef DFO_H
#define DFO_H

#include <vector>
#include <iostream>
#include <ctime>
#include <algorithm>

using namespace std;

class Point {
public:
    double x; // Total energy usage from previous timesteps
    double y; // Energy usage in the current timestep - either min or max

    Point(double x_val, double y_val);
    string to_string() const;
    // Overload the << operator for easy printing
    friend ostream& operator<<(ostream& os, const Point& point);
};

class DependencyPolygon {
public:
    vector<Point> points;
    double min_prev_energy;
    double max_prev_energy;
    int numsamples;

    DependencyPolygon(double min_prev, double max_prev, int numsamples);

    void generate_polygon(double charging_power, double next_min_prev, double next_max_prev);
    void add_point(double x, double y);
    void sort_points();
    void print_polygon(int index) const;
    string to_string() const;
    // Overload the << operator for easy printing
    friend ostream& operator<<(ostream& os, const DependencyPolygon& polygon);
};

class DFO {
public:
    int dfo_id;
    vector<DependencyPolygon> polygons;
    double charging_power;
    double min_total_energy;
    double max_total_energy;
    time_t earliest_start;
    time_t latest_start;

    DFO(int dfo_id, const vector<double>& min_prev, const vector<double>& max_prev, 
        int numsamples = 5, double charging_power = 7.3, double min_total_energy = -1, double max_total_energy = -1, time_t earliest_start = time(nullptr));

    void generate_dependency_polygons();
    void print_dfo() const;
    string to_string() const;
    // Overload the << operator for easy printing
    friend ostream& operator<<(ostream& os, const DFO& dfo);
};

#endif