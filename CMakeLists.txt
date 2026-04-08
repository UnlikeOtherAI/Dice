cmake_minimum_required(VERSION 3.24)
project(dice3d VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
add_subdirectory(core)
