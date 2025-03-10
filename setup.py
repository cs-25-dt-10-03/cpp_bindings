from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "flexoffer_logic",
        ["bindings.cpp", "src/clusters.cpp", "src/flexoffer.cpp", "src/groups.cpp", "src/helpers.cpp", "src/DFO.cpp"],
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

