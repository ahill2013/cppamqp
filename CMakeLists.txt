cmake_minimum_required(VERSION 2.8)

project(test)

include_directories(include)
include_directories(lib/rapidjson)
add_library(CommFrame SHARED src/mq.cpp src/processor.cpp src/linearizer.cpp)
set_property(TARGET CommFrame PROPERTY CXX_STANDARD 11)
set_target_properties(CommFrame PROPERTIES VERSION 1.0)
# set(warnings "-Wall -Westra -Werror")
# set(CMAKE_CXX_FLAGS "${warnings}")
