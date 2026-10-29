cmake_minimum_required(VERSION 3.20)
project(UniversalAncMeasurement CXX)

add_executable(anc_measurement
    anc_measurement.cpp
)

target_include_directories(anc_measurement PRIVATE
    ../../core/include
)

target_link_libraries(anc_measurement PRIVATE
    anc_core
)
