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
set(CLI_SOURCE_FILES
src/cli/rmap_packet.cc
)

include_directories(include)
add_library(spw STATIC ${SPW_SOURCE_FILES})
add_library(rmap STATIC ${RMAP_SOURCE_FILES})

add_executable(rmap-packet src/cli/rmap_packet.cc)
target_link_libraries(rmap-packet spw rmap)

add_subdirectory(test)