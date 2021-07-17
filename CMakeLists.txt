cmake_minimum_required(VERSION 3.0.0)

add_library(timer INTERFACE)

get_filename_component(TIMER_INCLUDE_PATH include ABSOLUTE)

target_include_directories(timer INTERFACE ${TIMER_INCLUDE_PATH})
target_compile_features(timer INTERFACE cxx_std_20)
