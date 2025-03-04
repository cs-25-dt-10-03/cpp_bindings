#ifndef FLEXOFFER_H
#define FLEXOFFER_H

#include <ctime>
#include <vector>

using namespace std;

class TimeSlice {
    public:
        double min_power; // Minimum power in kW
        double max_power; // Maximum power in kW
        TimeSlice(double min, double max);
};

class Flexoffer{
    private: 
        int offer_id;
        time_t earliest_start_time;
        time_t latest_start_time;
        time_t end_time;
        int duration; 
        vector<TimeSlice> profile;
        vector<double> scheduled_allocation;
        double min_overall_alloc;
        double max_overall_alloc;
        time_t scheduled_start_time;
    public:
        //Constructor
        Flexoffer(int oi, time_t est, time_t lst, time_t et, vector<TimeSlice> &p, int d, double min = 0, double max = 0);

        //Destructor
        virtual ~Flexoffer();

        // Getters
        int get_offer_id() const;
        time_t get_est() const;
        time_t get_lst() const;
        time_t get_et() const; 
        int get_duration() const;
        vector<TimeSlice> get_profile() const;
        vector<double> get_scheduled_allocation() const;
        time_t get_scheduled_start_time() const;

        // Setters
        void set_scheduled_allocation(std::vector<double>);
        void set_scheduled_start_time(time_t);

        // Additional methods
        int get_est_hour() const;
        int get_et_hour() const;
        int get_lst_hour() const;
        double get_total_energy() const;
        
        //Utils
        virtual void print_flexoffer();
};

#endif // FLEXOFFER_H

