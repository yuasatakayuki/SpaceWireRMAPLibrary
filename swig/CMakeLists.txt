####### CMakeLists.txt for RubySpaceWireRMAPLibrary

cmake_minimum_required(VERSION 2.8)

### Initial definition of cmake variables
set(CMAKE_INSTALL_PREFIX $ENV{HOME} CACHE PATH "install prefix")
set(CMAKE_BUILD_TYPE Release CACHE STRING "build type")
set(CMAKE_CXX_FLAGS_DEBUG "-g -W -Wall" CACHE STRING "CXX_FLAGS for debug")
set(CMAKE_C_FLAGS_DEBUG "-g -W -Wall" CACHE STRING "C_FLAGS for debug")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -W -Wall" CACHE STRING "CXX_FLAGS for release")
set(CMAKE_C_FLAGS_RELEASE "-O3 -W -Wall" CACHE STRING "C_FLAGS for release")

### Definition of project
project(RubySpaceWireRMAPLibrary)
message("-- Install destination prefix: ${CMAKE_INSTALL_PREFIX}")

### External libraries
set(CXXUTILITIES_DIR $ENV{CXXUTILITIES_PATH})
set(CXXUTILITIES_INC_DIR ${CXXUTILITIES_DIR}/includes)
set(XMLUTILITIES_DIR $ENV{XMLUTILITIES_PATH})
set(XMLUTILITIES_INC_DIR ${XMLUTILITIES_DIR}/include)
set(SPACEWIRE_RMAP_LIBRARY_DIR $ENV{SPACEWIRERMAPLIBRARY_PATH})
set(SPACEWIRE_RMAP_LIBRARY_INC_DIR ${SPACEWIRE_RMAP_LIBRARY_DIR}/includes)
set(XERCES_DIR $ENV{XERCESDIR})
set(XERCES_INC_DIR ${XERCES_DIR}/include)
set(XERCES_LIB_DIR ${XERCES_DIR}/lib)
set(XERCES_LIB xerces-c)

### subdirecties
add_subdirectory(ruby)

### END
