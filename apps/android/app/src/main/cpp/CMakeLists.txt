cmake_minimum_required(VERSION 3.22.1)
project("universal_anc_jni")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(anc_jni SHARED
    anc_jni.cpp
    ../../../../../../core/src/anc_core.cpp
)

target_include_directories(anc_jni PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../../../../../core/include
)

find_library(log-lib log)

target_link_libraries(anc_jni
    ${log-lib}
)
