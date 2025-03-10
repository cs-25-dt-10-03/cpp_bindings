#ifndef DFO_AGGREGATION_H
#define DFO_AGGREGATION_H

#include <vector>
#include <algorithm>
#include <ctime>
#include "DFO.h"

using namespace std;

class DFO_Aggregation {
public:
    static vector<DependencyPolygon> createStartPadding(int num_padding, int numsamples);
    static vector<DependencyPolygon> createEndPadding(const DFO& dfo, int num_padding, int numsamples);
    static vector<Point> findOrInterpolatePoints(const vector<Point>& points, double dependency_value);
    static double linearInterpolation(double x, double x0, double y0, double x1, double y1);

    static DFO agg2to1(const DFO& dfo1, const DFO& dfo2, int numsamples);
    static DFO aggnto1(const vector<DFO>& dfos, int numsamples);
};

#endif