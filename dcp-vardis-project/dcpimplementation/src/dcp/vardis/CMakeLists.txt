project(DCPStack-VarDis VERSION 1.3.6 DESCRIPTION "DCP Variable Dissemination Protocol" LANGUAGES CXX)

add_library(dcp-vardis SHARED
  vardisclient_configuration.cc
  vardisclient_lib.cc
  vardisclient_lib_rtdb.cc
  vardis_client_protocol_data.cc
  vardis_configuration.cc
  vardis_logging.cc
  vardis_management_command.cc
  vardis_management_rtdb.cc
  vardis_payload_handling.cc
  vardis_protocol_data.cc
  vardis_protocol_statistics.cc
  vardis_receiver.cc  
  vardis_scrubber.cc
  vardis_service_primitives.cc
  vardis_status.cc
  vardis_transmissible_types.cc  
  vardis_transmitter.cc)

  
add_executable (dcpmain-vardis vardis_main.cc)

# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\") with OMNeT++ support")
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  add_library(dcpsim-vardis SHARED
    vardis_protocol_data.cc
    vardis_payload_handling.cc
    vardis_protocol_statistics.cc
    vardis_status.cc
    vardis_service_primitives.cc
    vardis_transmissible_types.cc)
  target_include_directories(dcpsim-vardis PUBLIC ${CMAKE_SOURCE_DIR})
else()
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")
endif()

target_include_directories(dcp-vardis PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(dcpmain-vardis PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(dcp-vardis PUBLIC dcp-bp dcp-common)
target_link_libraries(dcpmain-vardis PRIVATE dcp-vardis)
