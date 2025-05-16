#include <ilcplex/ilocplex.h>
#include <iostream>
#include <vector>
#include <numeric>
#include <stdexcept>
#include "DFO.h"

using namespace std;

// Function to convert string to timestamp (you might have your own utility for this)
time_t stringToTimestamp(const string& dateString) {
    tm tm = {};
    if (strptime(dateString.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == nullptr) {
        throw runtime_error("Failed to parse date string: " + dateString);
    }
    time_t timestamp = mktime(&tm);
    if (timestamp == -1) {
        throw runtime_error("mktime failed to convert date string: " + dateString);
    }
    return timestamp;
}

// Function to add piecewise linear constraints, corrected logic.
void addPiecewiseLinearConstraints(
    IloModel& model,
    IloEnv& env,
    IloNumVar energy,
    IloExpr cumulativeEnergy,
    const vector<Point>& points,
    const string& constraintNamePrefix
) {
    if (points.empty()) {
        return; // No points, no constraints.
    }

    // Iterate through the *pairs* of points.
    for (size_t k = 0; k + 1 < points.size(); k += 2) {
        const Point& prevPointMin = points[k];
        const Point& prevPointMax = points[k + 1];
        const Point& nextPointMin = (k + 2 < points.size()) ? points[k + 2] : points[k]; // Handle last segment
        const Point& nextPointMax = (k + 3 < points.size()) ? points[k + 3] : points[k + 1]; // Handle last segment

        // Check if cumulativeEnergy is within the x-range of this segment.
        IloConstraint lowerBound = (cumulativeEnergy >= prevPointMin.x);
        IloConstraint upperBound = (cumulativeEnergy <= nextPointMin.x);

        if (prevPointMin.x <= nextPointMin.x) {
            // Add constraints for the valid range and break after finding the relevant range
            model.add(lowerBound);
            model.add(upperBound);

            // Linear interpolation for min and max energy
            IloExpr minEnergy(env);
            IloExpr maxEnergy(env);

            if (abs(nextPointMin.x - prevPointMin.x) > 1e-9) { // Avoid division by zero
                minEnergy = prevPointMin.y +
                            ((nextPointMin.y - prevPointMin.y) /
                             (nextPointMin.x - prevPointMin.x)) *
                            (cumulativeEnergy - prevPointMin.x);

                maxEnergy = prevPointMax.y +
                            ((nextPointMax.y - prevPointMax.y) /
                             (nextPointMax.x - prevPointMax.x)) *
                            (cumulativeEnergy - prevPointMax.x);
            } else {
                minEnergy = prevPointMin.y;
                maxEnergy = prevPointMax.y;
            }

            // Add constraints for energy
            model.add(energy >= minEnergy);
            model.add(energy <= maxEnergy);

            // End expressions to free memory
            minEnergy.end();
            maxEnergy.end();

            break; // Exit the loop after finding the correct segment
        }
        lowerBound.end();
        upperBound.end();
    }
}



vector<vector<double>> solveDFOOptimization(
    const vector<DFO>& dfos,
    const vector<vector<double>>& spotPrices,
    const vector<vector<double>>& reservePrices = {},
    const vector<vector<double>>& activationPrices = {},
    const vector<vector<pair<double, double>>>& indicators = {},
    const vector<vector<double>>& fixedP = {}
) {
    IloEnv env;
    vector<vector<double>> dfoSchedules; // Vector of vectors to hold schedules for each DFO.

    try {
        IloModel model(env);

        // 1. Offsets and Horizon
        time_t simStartTs = stringToTimestamp(config.SIMULATION_START_DATE);
        vector<int> offsets;
        for (const auto& dfo : dfos) {
            offsets.push_back(static_cast<int>((dfo.getEst() - simStartTs) / config.TIME_RESOLUTION));
        }
        size_t numDFOs = dfos.size();
        size_t T = spotPrices[0].size(); // Get horizon from the first market.  Assume all markets have same T.

        // 2. Decision Variables 
        vector<vector<IloNumVar>> energy(numDFOs); // energy[a][t]
        vector<vector<IloNumVar>> reserveUp(numDFOs);   // reserveUp[a][t]
        vector<vector<IloNumVar>> reserveDown(numDFOs); // reserveDown[a][t]
        vector<vector<IloNumVar>> activationUp(numDFOs);  // activationUp[a][t]
        vector<vector<IloNumVar>> activationDown(numDFOs); // activationDown[a][t]
        vector<vector<IloNumVar>> slackUp(numDFOs);       // slackUp[a][t]
        vector<vector<IloNumVar>> slackDown(numDFOs);     // slackDown[a][t]
        vector<vector<IloExpr>> cumulativeEnergy(numDFOs); // cumulativeEnergy[a][t] is now an *expression*.

        for (size_t a = 0; a < numDFOs; ++a) {
            energy[a].resize(T);
            cumulativeEnergy[a].resize(T, IloExpr(env)); // Initialize expressions.
            if (RUN_RESERVE) {
                reserveUp[a].resize(T);
                reserveDown[a].resize(T);
            }
            if (RUN_ACTIVATION) {
                activationUp[a].resize(T);
                activationDown[a].resize(T);
                slackUp[a].resize(T);
                slackDown[a].resize(T);
            }
            for (size_t t = 0; t < T; ++t) {
                energy[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("p_" + to_string(a) + "_" + to_string(t)).c_str());
                if (RUN_RESERVE) {
                    reserveUp[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("pr_up_" + to_string(a) + "_" + to_string(t)).c_str());
                    reserveDown[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("pr_dn_" + to_string(a) + "_" + to_string(t)).c_str());
                }
                if (RUN_ACTIVATION) {
                    activationUp[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("pb_up_" + to_string(a) + "_" + to_string(t)).c_str());
                    activationDown[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("pb_dn_" + to_string(a) + "_" + to_string(t)).c_str());
                    slackUp[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("s_up_" + to_string(a) + "_" + to_string(t)).c_str());
                    slackDown[a][t] = env.numVar(0.0, IloInfinity, ILOFLOAT, ("s_dn_" + to_string(a) + "_" + to_string(t)).c_str());
                }
            }
        }

        // 3. Build Objective (Corrected for multi-market and reserve/activation)
        IloExpr totalObjective(env);
        for (size_t a = 0; a < numDFOs; ++a) {
            for (size_t t = 0; t < T; ++t) {
                totalObjective -= spotPrices[0][t] * energy[a][t] * (TIME_RESOLUTION / 3600.0); //spot market.
                if (RUN_RESERVE && !reservePrices.empty()) {
                    totalObjective += (reservePrices[0][t] * reserveUp[a][t] + reservePrices[1][t] * reserveDown[a][t]) * (TIME_RESOLUTION / 3600.0); //reserve market
                }
                if (RUN_ACTIVATION && !activationPrices.empty() && !indicators.empty()) {
                    totalObjective += (activationPrices[0][t] * activationUp[a][t] + activationPrices[1][t] * activationDown[a][t]) * (TIME_RESOLUTION / 3600.0); //activation market
                    totalObjective -= PENALTY * (slackUp[a][t] + slackDown[a][t]) * (TIME_RESOLUTION/3600.0);
                }
            }
        }
        model.add(IloMinimize(env, totalObjective));
        totalObjective.end();

        // 4. Cumulative Energy Coupling (Corrected)
        for (size_t a = 0; a < numDFOs; ++a) {
            for (size_t t = 0; t < T; ++t) {
                if (t == 0) {
                    cumulativeEnergy[a][t] = energy[a][t] * (TIME_RESOLUTION / 3600.0); // Initialize at t=0
                }
                else {
                    cumulativeEnergy[a][t] = cumulativeEnergy[a][t-1] + energy[a][t] * (TIME_RESOLUTION / 3600.0); //cumulative energy
                }
            }
        }


        // 5. Piecewise Linear Dependency Constraints (CORRECTED)
        for (size_t a = 0; a < numDFOs; ++a) {
            const DFO& dfo = dfos[a];
            for (size_t j = 0; j < dfo.polygons.size(); ++j) {
                int t = offsets[a] + j;
                if (t < T) {
                    addPiecewiseLinearConstraints(model, env, energy[a][t], cumulativeEnergy[a][t-1], dfo.polygons[j].points, "pwl_" + to_string(a) + "_" + to_string(t));
                }
            }
        }

        // 6. Reserve and Activation Constraints (Corrected and extended)
        for (size_t a = 0; a < numDFOs; ++a) {
            for (size_t t = 0; t < T; ++t) {
                if (RUN_RESERVE) {
                    if (RUN_SPOT) {
                        model.add(reserveUp[a][t] <= energy[a][t]);
                        // max power at t.
                        IloExpr maxPowerExpr(env);
                        for(const auto& poly : dfos[a].polygons){
                            if(offsets[a] + static_cast<int>(dfos[a].polygons.size()) == t){
                                for(size_t p_index = 1; p_index < poly.points.size(); p_index+=2){
                                    maxPowerExpr += poly.points[p_index].y;
                                }
                            }
                        }
                        model.add(reserveDown[a][t] <= maxPowerExpr - energy[a][t]);
                        maxPowerExpr.end();

                    } else {
                        // max power at t.
                        IloExpr maxPowerExpr(env);
                        for(const auto& poly : dfos[a].polygons){
                            if(offsets[a] + static_cast<int>(dfos[a].polygons.size()) == t){
                                for(size_t p_index = 1; p_index < poly.points.size(); p_index+=2){
                                    maxPowerExpr += poly.points[p_index].y;
                                }
                            }
                         }
                        model.add(reserveUp[a][t] <= maxPowerExpr);
                        model.add(reserveDown[a][t] <= maxPowerExpr);
                        maxPowerExpr.end();
                    }
                }
                if (RUN_ACTIVATION && !indicators.empty()) {
                    model.add(activationUp[a][t] + slackUp[a][t] >= reserveUp[a][t] * indicators[0][t].first);
                    model.add(activationDown[a][t] + slackDown[a][t] >= reserveDown[a][t] * indicators[0][t].second);
                }
            }
        }

        // 7. Fix spot-only in sequential mode.
        if (!fixedP.empty()) {
            for (size_t a = 0; a < numDFOs; ++a) {
                for (size_t t = 0; t < T; ++t) {
                    if (fixedP[a].size() > t)
                    {
                        energy[a][t].setUB(fixedP[a][t]);
                        energy[a][t].setLB(fixedP[a][t]);
                    }
                   
                }
            }
        }

        // 8. Solve and Extract Solution
        IloCplex cplex(model);
        cplex.setOut(env.getNullStream()); // Suppress output for performance
        if (cplex.solve()) {
            for (size_t a = 0; a < numDFOs; ++a) {
                vector<double> dfoSchedule;
                for (size_t t = 0; t < T; ++t) {
                    dfoSchedule.push_back(cplex.getValue(energy[a][t]));
                }
                dfoSchedules.push_back(dfoSchedule);
            }
        }
        else {
            throw runtime_error("Failed to find optimal solution.");
        }

        // Clean up.  Important to free memory!
        for (size_t a = 0; a < numDFOs; ++a) {
            for (size_t t = 0; t < T; ++t) {
                energy[a][t].end();
                cumulativeEnergy[a][t].end();
                if (RUN_RESERVE) {
                    reserveUp[a][t].end();
                    reserveDown[a][t].end();
                }
                if (RUN_ACTIVATION) {
                    activationUp[a][t].end();
                    activationDown[a][t].end();
                    slackUp[a][t].end();
                    slackDown[a][t].end();
                }
            }
        }
    }
    catch (const IloException& e) {
        cerr << "CPLEX Error: " << e.getMessage() << endl;
        env.end();
        throw;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        env.end();
        throw;
    }
    catch (...) {
        cerr << "Unknown error occurred." << endl;
        env.end();
        throw;
    }
    env.end();
    return dfoSchedules;
}