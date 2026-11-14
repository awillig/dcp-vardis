project(DCPStack-SRP VERSION 2.0.0 DESCRIPTION "DCP State Reporting Protocol" LANGUAGES CXX)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/srp_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/srp_version.h"
    @ONLY
)

add_library(dcp-srp SHARED
  srpclient_configuration.cc
  srpclient_lib.cc
  srp_configuration.cc
  srp_logging.cc
  srp_protocol_name.cc
  srp_receiver.cc
  srp_scrubber.cc
  srp_status.cc
  srp_transmissible_types.cc
  srp_transmitter.cc)

add_executable (dcpmain-srp srp_main.cc)

# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\") with OMNeT++ support")
  add_library(dcpsim-srp SHARED
    srp_protocol_name.cc
    srp_status.cc
    srp_transmissible_types.cc)
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  target_include_directories(dcpsim-srp PUBLIC ${CMAKE_SOURCE_DIR})
  target_include_directories(dcpsim-srp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
else()
    message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")
endif()

target_include_directories(dcp-srp PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(dcp-srp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(dcpmain-srp PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(dcpmain-srp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")


target_link_libraries(dcp-srp PUBLIC dcp-bp dcp-common)
target_link_libraries(dcpmain-srp PRIVATE dcp-srp)
