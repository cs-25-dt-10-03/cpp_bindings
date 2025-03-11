#include "../include/config.h"
#include <iostream>
#include <fstream>
#include "../json.hpp"

using json = nlohmann::json;

int TIME_RESOLUTION = 3600; //fallback to 1 hour

void loadConfig() {
    const std::string path = "../master-thesis/config.json";

    std::ifstream file(path);
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
