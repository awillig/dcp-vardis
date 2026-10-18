project(DCPStack-Common VERSION 1.3.6 DESCRIPTION "DCP Common" LANGUAGES CXX)

add_library (dcp-common SHARED
  command_socket.cc
  configuration.cc
  debug_helpers.cc
  global_types_constants.cc
  logging_helpers.cc
  other_helpers.cc
  sharedmem_configuration.cc
  sharedmem_structure_base.cc)


# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\") with OMNeT++ support")
  add_library(dcpsim-common SHARED
    debug_helpers.cc
    global_types_constants.cc)
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  target_include_directories(dcpsim-common PUBLIC ${CMAKE_SOURCE_DIR})
else()
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")
endif()

target_include_directories(dcp-common PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(dcp-common ${PROJECT_LIB})
