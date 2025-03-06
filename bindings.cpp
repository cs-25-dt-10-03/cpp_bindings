#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "include/clusters.h"
#include "include/helpers.h"

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
        .def("get_scheduled_allocation", &Flexoffer::get_scheduled_allocation)
        .def("get_scheduled_start_time", &Flexoffer::get_scheduled_start_time)
        .def("set_scheduled_allocation", &Flexoffer::set_scheduled_allocation)
        .def("set_scheduled_start_time", &Flexoffer::set_scheduled_start_time)
        .def("print_flexoffer", &Flexoffer::print_flexoffer)
        .def("get_est_hour", &Flexoffer::get_est_hour)
        .def("get_lst_hour", &Flexoffer::get_lst_hour)
        .def("get_et_hour", &Flexoffer::get_et_hour)
        .def("get_total_energy", &Flexoffer::get_total_energy);

    pybind11::class_<Fo_Group>(m, "Fo_Group")
        .def(pybind11::init<int>()) 
        .def("getFlexOffers", &Fo_Group::getFlexOffers)
        .def("addFlexOffer", &Fo_Group::addFlexOffer);

    m.def("clusterFo_Group", &clusterFo_Group, "Clusters a vector of Fo_Group",
          pybind11::arg("groups"), pybind11::arg("est_threshold"),
          pybind11::arg("lst_threshold"),
          pybind11::arg("max_group_size"));


    m.def("start_alignment_aggregate", &start_alignment_aggregate,
        "Perform start alignment aggregation for FlexOffers, handling min/max energy profiles.\n"
        "Inputs:\n"
        "  - min_profiles: list of min energy profiles (list of list of doubles)\n"
        "  - max_profiles: list of max energy profiles (list of list of doubles)\n"
        "  - earliest: list of earliest start times (ints)\n"
        "  - latest: list of latest start times (ints)\n"
        "  - offsets: list of offsets (ints)\n"
        "Returns a dict with aggregated profiles and time window.",
        py::arg("min_profiles"), py::arg("max_profiles"),
        py::arg("earliest"), py::arg("latest"), py::arg("offsets"));
}

