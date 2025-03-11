#ifndef DFO_DISAGGREGATION_H
#define DFO_DISAGGREGATION_H

#include "DFO.h"
#include "DFO_aggregation.h"
#include <vector>

using namespace std;

class DFO_disaggregation {
public:
    static pair<vector<double>, vector<double>> disagg1to2(
        const DFO& D1, const DFO& D2, const DFO& DA, const vector<double>& yA_ref);

    static vector<vector<double>> disagg1toN(
        const DFO& DA, const vector<DFO>& DFOs, const vector<double>& yA_ref);
};

#endif