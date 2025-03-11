from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "flexoffer_logic",
        ["bindings.cpp", "src/clusters.cpp", "src/flexoffer.cpp", "src/groups.cpp", "src/helpers.cpp", 
         "src/DFO.cpp", "src/DFO_aggregation.cpp", "src/config.cpp"],
=======
        ["bindings.cpp", "src/clusters.cpp", "src/flexoffer.cpp", "src/groups.cpp", "src/helpers.cpp", "src/DFO.cpp", "src/DFO_aggregation.cpp", "src/DFO:disaggregation.cpp"],
>>>>>>> d40cb585b56c1a69d55c7bd654cc807274a9ec17
        include_dirs=[pybind11.get_include(), "../include"],  
        language="c++",
    ),
]

setup(
    name="flexoffer_logic",
    version="0.1",
    ext_modules=ext_modules,
    zip_safe=False,
)

