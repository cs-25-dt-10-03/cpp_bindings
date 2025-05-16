#ifndef CONFIG_H
#define CONFIG_H

#include <string>

using namespace std;

extern int TIME_RESOLUTION;
extern double PENALTY;
extern string SIMULATION_START_DATE;
extern bool RUN_RESERVE;
extern bool RUN_ACTIVATION;
extern bool RUN_SPOT;

void loadConfig();
void set_time_resolution(int);

#endif
