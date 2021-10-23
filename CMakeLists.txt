cmake_minimum_required(VERSION 3.10)

project(SpaceWireRMAPLibrary CXX)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(SPW_SOURCE_FILES
src/spacewire/spacewiressdtpmodule.cc
src/spacewire/spacewireutil.cc
)
set(RMAP_SOURCE_FILES
src/rmap/rmapengine.cc
src/rmap/rmapinitiator.cc
src/rmap/rmappacket.cc
src/rmap/rmapreplystatus.cc
)

include_directories(include)
add_library(spw STATIC ${SPW_SOURCE_FILES})
add_library(rmap STATIC ${RMAP_SOURCE_FILES})


