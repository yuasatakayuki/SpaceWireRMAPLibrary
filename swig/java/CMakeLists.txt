########################################################
set(PROJECTNAME SpaceWireRMAP)
set(TARGET_EXT_LIBRARY lib${PROJECTNAME})
set(SWIG_IF_FILE ${PROJECTNAME}.i)
########################################################

set(XERCES_LIB "xerces-c")

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

include_directories(
  $ENV{CXXUTILITIES_PATH}/includes
  $ENV{XMLUTILITIES_PATH}/include
  $ENV{SPACEWIRERMAPLIBRARY_PATH}/includes
  $ENV{XERCES_DIR}/include
  $ENV{JAVA_HOME}/Headers
)

link_directories(
  ${XERCES_DIR}/lib
)

### compile option (Debug) 
set(CMAKE_CXX_FLAGS_DEBUG "-W -g")
set(CMAKE_C_FLAGS_DEBUG "-W -g")

### compile option (Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_RELEASE "-O2")

set(CMAKE_SWIG_FLAGS "")

set_source_files_properties(${SWIG_IF_FILE} PROPERTIES CPLUSPLUS ON)
# set_source_files_properties(${SWIG_IF_FILE} PROPERTIES SWIG_FLAGS "-includeall")
add_definitions(-DHAVE_STRUCT_TIMESPEC -DHAVE_SIGNBIT)
swig_add_module(${TARGET_EXT_LIBRARY} java ${SWIG_IF_FILE})
swig_link_libraries(${TARGET_EXT_LIBRARY} ${XERCES_LIB})

set_target_properties(${TARGET_EXT_LIBRARY}
  PROPERTIES
  PREFIX ""
  )

install(TARGETS ${TARGET_EXT_LIBRARY}
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/java)
