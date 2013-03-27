########################################################
set(TARGET_EXT_LIBRARY SpaceWireRMAP)
set(SWIG_IF_FILE ${TARGET_EXT_LIBRARY}.i)
########################################################

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

find_package(Ruby)
set(RUBY_LINK_FLAGS " ")
if(${RUBY_VERSION} VERSION_GREATER 1.9.0)
  set(RUBY_INCLUDE_DIRS ${RUBY_HDR_DIR})
  set(RUBY_LIBRARY "")
  if(APPLE)
    set(RUBY_LINK_FLAGS "-Wl,-undefined,dynamic_lookup -Wl,-multiply_defined,suppress -Wl,-flat_namespace")
  endif()
endif()

message("-- Ruby include: ${RUBY_INCLUDE_DIRS}")
message("-- Ruby library: ${RUBY_LIBRARY}")

include_directories(
  ${CXXUTILITIES_INC_DIR}
  ${XMLUTILITIES_INC_DIR}
  ${SPACEWIRE_RMAP_LIBRARY_INC_DIR}
  ${XERCES_INC_DIR}
  ${RUBY_INCLUDE_DIRS}
)

link_directories(
  ${XERCES_LIB_DIR}
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
swig_add_module(${TARGET_EXT_LIBRARY} ruby ${SWIG_IF_FILE})
swig_link_libraries(${TARGET_EXT_LIBRARY} ${RUBY_LIBRARY} ${XERCES_LIB})

if(APPLE)
  set(RUBY_BINDING_SUFFIX ".bundle")
else(APPLE)
  set(RUBY_BINDING_SUFFIX ".so")
endif(APPLE)

set_target_properties(${TARGET_EXT_LIBRARY}
  PROPERTIES
  PREFIX ""
  SUFFIX ${RUBY_BINDING_SUFFIX}
  LINK_FLAGS ${RUBY_LINK_FLAGS}
  )

install(TARGETS ${TARGET_EXT_LIBRARY}
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/ruby)
