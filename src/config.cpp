#include "../include/config.h"
#include <iostream>
#include <fstream>
#include "../json.hpp"

using namespace std;
using json = nlohmann::json;

int TIME_RESOLUTION = 3600; //fallback to 1 hour

void loadConfig() {
    const string path = "../master_thesis/config.json";
    ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open " << path << std::endl;
        return;
    }

    json config;
    file >> config;

    if (config.contains("TIME_RESOLUTION")) {
        TIME_RESOLUTION = config["TIME_RESOLUTION"];
        std::cout << "TIME_RESOLUTION set to " << TIME_RESOLUTION << " seconds\n";
    }
}

struct ConfigInitializer {
    ConfigInitializer() {
        loadConfig();
    }
};

static ConfigInitializer configInit;
