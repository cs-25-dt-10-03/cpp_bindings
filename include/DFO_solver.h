#ifndef SOLVER_H
#define SOLVER_H

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include "DFO.h"
#include "config.h"

class IloEnv;
class IloModel;
class IloNumVar;
class IloExpr;
class IloConstraint;

using namespace std;

time_t stringToTimestamp(const string& dateString);

void addPiecewiseLinearConstraints(
    IloModel& model,
    IloEnv& env,
    IloNumVar energy,
    IloExpr cumulativeEnergy,
    const vector<Point>& points,
    const string& constraintNamePrefix
);

vector<vector<double>> solveDFOOptimization(
    const vector<DFO>& dfos,
    const vector<vector<double>>& spotPrices,
    const vector<vector<double>>& reservePrices = {},
    const vector<vector<double>>& activationPrices = {},
    const vector<vector<pair<double, double>>>& indicators = {},
    const vector<vector<double>>& fixedP = {}
);

#endif // SOLVER_H