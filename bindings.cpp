#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "include/clusters.h"
#include "include/alignments.h"
#include "include/helpers.h"
#include "include/DFO.h"
#include "include/config.h"
#include "include/DFO_aggregation.h"
#include "include/DFO_disaggregation.h"

PYBIND11_MODULE(flexoffer_logic, m) {
    pybind11::class_<TimeSlice>(m, "TimeSlice")
        .def(pybind11::init<double, double>())
        .def_readwrite("min_power", &TimeSlice::min_power)
        .def_readwrite("max_power", &TimeSlice::max_power);

    pybind11::class_<Flexoffer>(m, "Flexoffer")
        .def(pybind11::init<int, time_t, time_t, time_t, std::vector<TimeSlice>&, int, double, double>(),
             pybind11::arg("offer_id"), pybind11::arg("earliest_start"), pybind11::arg("latest_start"),
             pybind11::arg("end_time"), pybind11::arg("profile"), pybind11::arg("duration"),
             pybind11::arg("min_overall_alloc") = 0, pybind11::arg("max_overall_alloc") = 0)
        .def("get_offer_id", &Flexoffer::get_offer_id)
        .def("get_est", &Flexoffer::get_est)
        .def("get_lst", &Flexoffer::get_lst)
        .def("get_et", &Flexoffer::get_et)
        .def("get_duration", &Flexoffer::get_duration)
        .def("get_profile", &Flexoffer::get_profile)
        .def("get_min_overall_alloc", &Flexoffer::get_min_overall_alloc)
        .def("get_max_overall_alloc", &Flexoffer::get_max_overall_alloc)
        .def("get_scheduled_allocation", &Flexoffer::get_scheduled_allocation)
        .def("get_scheduled_start_time", &Flexoffer::get_scheduled_start_time)
        .def("set_scheduled_allocation", &Flexoffer::set_scheduled_allocation)
        .def("set_scheduled_start_time", &Flexoffer::set_scheduled_start_time)
        .def("set_profile", &Flexoffer::set_profile)
        .def("print_flexoffer", &Flexoffer::print_flexoffer)
        .def("get_est_hour", &Flexoffer::get_est_hour)
        .def("get_lst_hour", &Flexoffer::get_lst_hour)
        .def("get_et_hour", &Flexoffer::get_et_hour)
        .def("get_total_energy", &Flexoffer::get_total_energy)
        .def("get_allowed_start_times", &Flexoffer::get_allowed_start_times);
    
    pybind11::class_<Point>(m, "Point")
        .def(pybind11::init<double, double>())
        .def_readwrite("x", &Point::x)
        .def_readwrite("y", &Point::y)
        .def("__repr__", &Point::to_string);

    pybind11::class_<DependencyPolygon>(m, "DependencyPolygon")
        .def(pybind11::init<double, double, int>())
        .def_readwrite("min_prev_energy", &DependencyPolygon::min_prev_energy)
        .def_readwrite("max_prev_energy", &DependencyPolygon::max_prev_energy)
        .def_readwrite("numsamples", &DependencyPolygon::numsamples)
        .def_readwrite("points", &DependencyPolygon::points)
        .def("generate_polygon", &DependencyPolygon::generate_polygon)
        .def("add_point", &DependencyPolygon::add_point)
        .def("sort_points", &DependencyPolygon::sort_points)
        .def("__repr__", &DependencyPolygon::to_string);

    pybind11::class_<DFO>(m, "DFO")
        .def(pybind11::init<int, const std::vector<double>&, const std::vector<double>&, 
                      int, double, double, double, std::time_t>(),
             pybind11::arg("dfo_id"), pybind11::arg("min_prev"), pybind11::arg("max_prev"),
             pybind11::arg("numsamples") = 5, pybind11::arg("charging_power") = 7.3, 
             pybind11::arg("min_total_energy") = -1, pybind11::arg("max_total_energy") = -1, 
             pybind11::arg("earliest_start_time") = std::time(nullptr))
        .def_readwrite("dfo_id", &DFO::dfo_id)
        .def_readwrite("charging_power", &DFO::charging_power)
        .def_readwrite("min_total_energy", &DFO::min_total_energy)
        .def_readwrite("max_total_energy", &DFO::max_total_energy)
        .def_readwrite("earliest_start_time", &DFO::earliest_start_time)
        .def_readwrite("latest_start_time", &DFO::latest_start_time)
        .def_readwrite("end_time", &DFO::end_time)
        .def_readwrite("polygons", &DFO::polygons)
        .def("generate_dependency_polygons", &DFO::generate_dependency_polygons)
        .def("calculate_latest_start_time", &DFO::calculate_latest_start_time)
        .def("get_est_hour", &DFO::get_est_hour)
        .def("get_lst_hour", &DFO::get_lst_hour)
        .def("get_et_hour", &DFO::get_et_hour)
        .def("get_est", &DFO::get_est)
        .def("get_lst", &DFO::get_lst)
        .def("get_et", &DFO::get_et)
        .def("__repr__", &DFO::to_string);

    m.def("agg2to1", &DFO_Aggregation::agg2to1, "Aggregate two DFOs into one, accounting for different start times",
        pybind11::arg("dfo1"), pybind11::arg("dfo2"), pybind11::arg("numsamples"));
  
    m.def("aggnto1", &DFO_Aggregation::aggnto1, "Aggregate multiple DFOs into one, accounting for different start times",
        pybind11::arg("dfos"), pybind11::arg("numsamples"));
  
    m.def("findOrInterpolatePoints", &DFO_Aggregation::findOrInterpolatePoints, 
        "Finds or interpolate points for a given dependency value",
        pybind11::arg("points"), pybind11::arg("dependency_value"));

    m.def("disagg1to2", &DFO_disaggregation::disagg1to2, "Disaggregate a single aggregated DFO into two",
        pybind11::arg("D1"), pybind11::arg("D2"), pybind11::arg("DA"), pybind11::arg("yA_ref"));
      
    m.def("disagg1toN", &DFO_disaggregation::disagg1toN, "Disaggregate a single aggregated DFO into multiple DFOs",
        pybind11::arg("DA"), pybind11::arg("DFOs"), pybind11::arg("yA_ref"));

    pybind11::class_<Fo_Group>(m, "Fo_Group")
        .def(pybind11::init<int>()) 
        .def("getFlexOffers", &Fo_Group::getFlexOffers)
        .def("addFlexOffer", &Fo_Group::addFlexOffer);

    m.def("clusterFo_Group", &clusterFo_Group, "Clusters a vector of Fo_Group",
        pybind11::arg("groups"), pybind11::arg("est_threshold"),
        pybind11::arg("lst_threshold"),
        pybind11::arg("max_group_size"));

    m.def("start_alignment_aggregate", &start_alignment_aggregate, "Aggregate FlexOffers using start alignment.",
        pybind11::arg("flex_offers"));
    
    m.def("balance_alignment_aggregate", &balance_alignment_aggregate, "Aggregate FlexOffers using balance alignment.",
        pybind11::arg("flex_offers"), 
        pybind11::arg("num_candidates"));


    m.attr("TIME_RESOLUTION") = TIME_RESOLUTION;
    m.def("reload_config", &loadConfig, "Reload configuration from config.json");
    
}

