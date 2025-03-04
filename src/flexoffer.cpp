#include "../include/flexoffer.h"

#include <iostream>
#include <iomanip>

using namespace std;

TimeSlice::TimeSlice(double min, double max){
    min_power = min;
    max_power = max;
}

Flexoffer::Flexoffer(int oi, time_t est, time_t lst, time_t et,
                     vector<TimeSlice> &p, int d, double min, double max){
    offer_id = oi;
    earliest_start_time = est;
    latest_start_time = lst;
    duration = d;
    profile = p;
    end_time = et;
    scheduled_allocation.resize(duration, 0.0);
    scheduled_start_time = est;
    min_overall_alloc = min;
    max_overall_alloc = max;
};

//Destructor
Flexoffer::~Flexoffer(){};

//Getters
int Flexoffer::get_offer_id() const {return offer_id;};
time_t Flexoffer::get_est() const {return earliest_start_time;};
time_t Flexoffer::get_lst() const {return latest_start_time;};
time_t Flexoffer::get_et() const {return end_time;} // Now implemented
int Flexoffer::get_duration() const {return duration;};
vector<TimeSlice> Flexoffer::get_profile() const {return profile;};
vector<double> Flexoffer::get_scheduled_allocation() const {return scheduled_allocation;};
time_t Flexoffer::get_scheduled_start_time() const {return scheduled_start_time;};

//Setters
void Flexoffer::set_scheduled_allocation(vector<double> new_sa) {scheduled_allocation = new_sa;};
void Flexoffer::set_scheduled_start_time(time_t new_st) {scheduled_start_time = new_st;};

void Flexoffer::print_flexoffer() {
    // Helper lambda to convert time_t to readable format
    auto to_readable = [](time_t timestamp) -> string {
        char buffer[20];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return string(buffer);
    };

    cout << "=== FlexOffer Details ===" << std::endl;
    cout << "Offer ID: " << offer_id << std::endl;
    cout << "Earliest Start Time: " << to_readable(earliest_start_time) << std::endl;
    cout << "Latest Start Time:   " << to_readable(latest_start_time) << std::endl;
    cout << "Latest End Time:     " << to_readable(end_time) << std::endl;
    cout << "Duration:            " << duration << " hour(s)" << std::endl;
    cout << "Profile Elements:" << std::endl;

    for (int i = 0; i < duration; i++) {
        cout << "  Hour " << i << ": Min Power = " << std::fixed << std::setprecision(2)
                  << profile[i].min_power << " kW, Max Power = "
                  << profile[i].max_power << " kW" << endl;
    }
    cout << "Scheduled Allocation:" << endl;
    for (int i = 0; i < duration; i++) {

        time_t t = latest_start_time + i * 3600;
        cout << "  Hour " << i << " (" << to_readable(t) << "): "
             << "Power=" << scheduled_allocation[i] << " kW" << endl;
    }
    cout << "==========================" << endl;
}

// Additional methods
int Flexoffer::get_est_hour() const {
    struct tm* timeinfo = localtime(&earliest_start_time);
    return timeinfo->tm_hour;
}

int Flexoffer::get_lst_hour() const {
    struct tm* timeinfo = localtime(&latest_start_time);
    return timeinfo->tm_hour;
}

int Flexoffer::get_et_hour() const {
    struct tm* timeinfo = localtime(&end_time);
    return timeinfo->tm_hour;
}

double Flexoffer::get_total_energy() const {
    double total_energy = 0.0;
    for (const auto& ts : profile) {
        double avg_power = (ts.min_power + ts.max_power) / 2.0;
        total_energy += avg_power * 1.0; // Assuming each TimeSlice represents 1 hour
    }
    return total_energy;
}


