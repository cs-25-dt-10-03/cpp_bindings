#include "../include/flexoffer_disaggregation.h"
#include <cmath>

vector<Flexoffer> dissaggregate_flexoffers(Flexoffer fo, vector<Flexoffer> individual_offers){
    vector<double> fraction(fo.get_duration(), 0.0);
    vector<TimeSlice> profile = fo.get_profile();
    vector<double> scheduled_allocation = fo.get_scheduled_allocation();

    for (int i = 0; i < fo.get_duration(); i++) {
        double denom = profile[i].max_power - profile[i].min_power;
        fraction[i] = (scheduled_allocation[i] - profile[i].min_power) / denom;
        if (fraction[i] < 0.0) fraction[i] = 0.0;
        if (fraction[i] > 1.0) fraction[i] = 1.0;
    }

    // Disaggregate to each original Flexoffer
    vector<Flexoffer> result;
    time_t aggregator_start = fo.get_scheduled_start_time();

    for (auto &vfo : individual_offers) {
        Flexoffer f = (vfo);

        double start_diff_sec = difftime(f.get_est(), aggregator_start);
        int offSetHour = (int)std::floor(start_diff_sec / 3600.0);

        vector<double> f_scheduled_allocation((size_t)f.get_duration(), 0.0);
        auto f_profile = f.get_profile();

        for (int h = 0; h < f.get_duration(); h++) {
            int idx = offSetHour + h;
            if (idx >= 0 && idx < fo.get_duration()) {
                double f_min = f_profile[h].min_power;
                double f_max = f_profile[h].max_power;
                double denom = f_max - f_min;
                f_scheduled_allocation[h] = f_min + denom * fraction[idx];
            }
        }
        time_t f_scheduled_start = aggregator_start + (offSetHour * 3600);

        f.set_scheduled_allocation(f_scheduled_allocation);
        f.set_scheduled_start_time(f_scheduled_start);

        result.push_back(f);
    }

    return result;
}
