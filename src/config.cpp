#include "../include/config.h"
#include <iostream>
#include <fstream>
#include "../json.hpp"

using namespace std;
using json = nlohmann::json;

int TIME_RESOLUTION;
double PENALTY = 1000.0;
string SIMULATION_START_DATE = "";
bool RUN_RESERVE = false;
bool RUN_ACTIVATION = false;
bool RUN_SPOT = true;

void loadConfig() {
    const string path = "../master_thesis/config.json";
    ifstream file(path);
    if (!file) {
        cerr << "Error: Could not open " << path << endl;
        return;
    }

    json config;
    file >> config;

    if (config.contains("TIME_RESOLUTION")) {
        TIME_RESOLUTION = config["TIME_RESOLUTION"];
        cout << "TIME_RESOLUTION set to " << TIME_RESOLUTION << " seconds\n";
    }
    if (config.contains("PENALTY")) {
        PENALTY = config["PENALTY"];
        cout << "PENALTY set to " << PENALTY << "\n";
    }
    if (config.contains("SIMULATION_START_DATE")) {
        SIMULATION_START_DATE = config["SIMULATION_START_DATE"].get<string>();
        cout << "SIMULATION_START_DATE set to " << SIMULATION_START_DATE << "\n";
    }
    if (config.contains("RUN_RESERVE")) {
        RUN_RESERVE = config["RUN_RESERVE"];
        cout << "RUN_RESERVE set to " << (RUN_RESERVE ? "true" : "false") << "\n";
    }
    if (config.contains("RUN_ACTIVATION")) {
        RUN_ACTIVATION = config["RUN_ACTIVATION"];
        cout << "RUN_ACTIVATION set to " << (RUN_ACTIVATION ? "true" : "false") << "\n";
    }
    if (config.contains("RUN_SPOT")) {
        RUN_SPOT = config["RUN_SPOT"];
        cout << "RUN_SPOT set to " << (RUN_SPOT ? "true" : "false") << "\n";
    }
}

struct ConfigInitializer {
    ConfigInitializer() {
        loadConfig();
    }
};

static ConfigInitializer configInit;

void set_time_resolution(int timeRes) {
    TIME_RESOLUTION = timeRes;
    cout << "TIME RESOLUTION: " <<TIME_RESOLUTION << "\n";
}
