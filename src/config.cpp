#include "../include/config.h"
#include <iostream>
#include <fstream>
#include "../json.hpp"

using namespace std;
using json = nlohmann::json;

int TIME_RESOLUTION = 3600;

void set_time_resolution(int timeRes) {
    cout << "TIME RESOLUTION BEFORE: " << TIME_RESOLUTION << "\n" << "what is provided to the set time resolution" << timeRes << "\n";
    TIME_RESOLUTION = timeRes;
    cout << "TIME RESOLUTION AFTER: " <<TIME_RESOLUTION << "\n";
}

int get_time_resolution() {
    return TIME_RESOLUTION;
}
