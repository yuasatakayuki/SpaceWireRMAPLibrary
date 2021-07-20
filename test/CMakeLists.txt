cmake_minimum_required(VERSION 3.10)

project(srl_test)

set (CMAKE_CXX_STANDARD 11)

include_directories(../includes)
include_directories(/usr/local/include)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

add_executable(testrmapengine testrmapengine.cc)
# target_link_directories(testrmapengine /usr/local/lib)

target_link_libraries(testrmapengine xerces-c)