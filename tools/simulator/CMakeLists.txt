cmake_minimum_required(VERSION 3.20)
project(UniversalAncSimulator CXX)

add_executable(anc_simulator
    anc_simulator.cpp
)

target_include_directories(anc_simulator PRIVATE
    ../../core/include
)

target_link_libraries(anc_simulator PRIVATE
    anc_core
)

add_executable(tonal_pipeline_test tonal_pipeline_test.cpp)
target_include_directories(tonal_pipeline_test PRIVATE ../../apps/windows/include)
target_link_libraries(tonal_pipeline_test PRIVATE anc_core)
add_executable(anc_adaptive anc_adaptive.cpp)
target_link_libraries(anc_adaptive PRIVATE anc_core)
