project(DCPStack-BP VERSION 1.3.6 DESCRIPTION "DCP Beaconing Protocol" LANGUAGES CXX)


add_library(dcp-bp SHARED
  bpclient_lib.cc
  bpclient_lib_payload.cc
  bp_client_protocol_data.cc
  bp_client_static_info.cc
  bp_configuration.cc
  bp_logging.cc
  bp_management_command.cc
  bp_management_payload.cc
  bp_queueing_mode.cc
  bp_receiver.cc
  bp_runtime_data.cc
  bp_service_primitives.cc
  bp_shm_control_segment.cc
  bp_status.cc
  bp_transmissible_types.cc
  bp_transmitter.cc)

add_executable (dcpmain-bp bp_main.cc)

# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\") with OMNeT++ support")
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  add_library(dcpsim-bp SHARED
    bp_queueing_mode.cc
    bp_status.cc
    bp_service_primitives.cc
    bp_client_static_info.cc
    bp_transmissible_types.cc)
  target_include_directories(dcpsim-bp PUBLIC ${CMAKE_SOURCE_DIR})
else()
    message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")
endif()

target_include_directories(dcp-bp PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(dcpmain-bp PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(dcp-bp PUBLIC dcp-common tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(dcpmain-bp PRIVATE dcp-bp)
