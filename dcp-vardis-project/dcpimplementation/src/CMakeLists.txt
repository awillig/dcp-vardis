# global settings
cmake_minimum_required(VERSION 3.25.1)
project(DcpVarDis VERSION 1.3.0 DESCRIPTION "DCP/VarDis Linux Implementation" LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -g -fPIC -fopenmp -DBOOST_LOG_DYN_LINK -I.")

set(CMAKE_INCLUDE_CURRENT_DIR ON)


# ========================================================================================
# Main outputs
# ========================================================================================

file(GLOB common_SRC
  "dcp/common/*.h"
  "dcp/common/*.cc"
)

file (GLOB bp_SRC
  "dcp/bp/*.h"
  "dcp/bp/*.cc"
)

file (GLOB srp_SRC
  "dcp/srp/*.h"
  "dcp/srp/*.cc"
)

file (GLOB vardis_SRC
  "dcp/vardis/*.h"
  "dcp/vardis/*.cc"
)

file (GLOB applications_SRC
  "dcp/applications/*.h"
  "dcp/applications/*.cc"
)

# targets for implementation, linked against BOOST
add_library(dcplib-common SHARED ${common_SRC})
add_library(dcplib-bp SHARED ${bp_SRC})
add_library(dcplib-srp SHARED ${srp_SRC})
add_library(dcplib-vardis SHARED ${vardis_SRC})
add_executable(dcpmain-bp "dcp/main/bp_main.cc")
add_executable(dcpmain-srp "dcp/main/srp_main.cc")
add_executable(dcpmain-vardis "dcp/main/vardis_main.cc")
add_executable(vardisapp-test-producer "dcp/applications/vardisapp-test-producer.cc")
add_executable(vardisapp-test-consumer "dcp/applications/vardisapp-test-consumer.cc")
add_executable(vardisapp-delete-variable "dcp/applications/vardisapp-delete-variable.cc")
add_executable(vardisapp-describe-variable "dcp/applications/vardisapp-describe-variable.cc")
add_executable(vardisapp-list-variables "dcp/applications/vardisapp-list-variables.cc")
add_executable(srpapp-test-generate-sd "dcp/applications/srpapp-test-generate-sd.cc")
add_executable(srpapp-display-neighbour-table "dcp/applications/srpapp-display-neighbour-table.cc")

# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message("Building libraries suitable for OMNeT++, OPP_INCLUDE_PATH=${OPP_INCLUDE_PATH}")
  add_library(dcplib-common-sim SHARED
    dcp/common/debug_helpers.cc
    dcp/common/global_types_constants.cc
    dcp/common/services_status.cc)
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  add_library(dcplib-bp-sim SHARED
    dcp/bp/bp_queueing_mode.cc
    dcp/bp/bp_service_primitives.cc
    dcp/bp/bp_client_static_info.cc
    dcp/bp/bp_transmissible_types.cc)
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  add_library(dcplib-vardis-sim SHARED
    dcp/vardis/vardis_protocol_data.cc
    dcp/vardis/vardis_protocol_statistics.cc
    dcp/vardis/vardis_service_primitives.cc
    dcp/vardis/vardis_transmissible_types.cc)
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
endif()

find_package(Boost COMPONENTS program_options log log_setup REQUIRED)
set(PROJECT_LIB ${PROJECT_LIB} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
find_library(TINS_LIB tins)
find_library(NCURSES_LIB ncurses)

target_link_libraries(dcplib-bp dcplib-common tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(dcplib-srp dcplib-bp)
target_link_libraries(dcplib-vardis dcplib-bp)
target_link_libraries(dcpmain-bp -Wl,--start-group  ${PROJECT_LIB} tins dcplib-common dcplib-bp -Wl,--end-group)
target_link_libraries(dcpmain-srp -Wl,--start-group tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-bp dcplib-srp -Wl,--end-group)
target_link_libraries(dcpmain-vardis -Wl,--start-group tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-bp dcplib-vardis -Wl,--end-group)
target_link_libraries(vardisapp-test-producer -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-vardis -Wl,--end-group)
target_link_libraries(vardisapp-test-consumer -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-vardis ncurses -Wl,--end-group)
target_link_libraries(vardisapp-delete-variable -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-vardis -Wl,--end-group)
target_link_libraries(vardisapp-describe-variable -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-vardis -Wl,--end-group)
target_link_libraries(vardisapp-list-variables -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-vardis -Wl,--end-group)
target_link_libraries(srpapp-test-generate-sd -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-srp -Wl,--end-group)
target_link_libraries(srpapp-display-neighbour-table -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcplib-common dcplib-srp ncurses -Wl,--end-group)


# ========================================================================================
# Testing
# ========================================================================================

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2 
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()


# ========================================================================================
# Documentation (doxygen)
# ========================================================================================

find_package (Doxygen REQUIRED dot mscgen dia)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.config)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen.config)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen needs to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

# ========================================================================================
# Defining all the outputs
# ========================================================================================


add_executable(bp_shm_test "test/bp/bp_client_sharedmem_test.cc")
add_executable(common_tt_test "test/common/transmissible_types_test.cc")
add_executable(common_shm_test "test/common/shared_mem_area_test.cc")
add_executable(common_ser_test "test/common/serialization_area_test.cc")
add_executable(common_cs_test "test/common/command_socket_test.cc")
add_executable(common_avl_test "test/common/avl_tree.cc")
add_executable(common_misc_test "test/common/miscellaneous_test.cc")
add_executable(vardis_tt_test "test/vardis/vardis_transmissible_types_test.cc")
add_executable(vardis_pd_test "test/vardis/vardis_protocol_data_test.cc")
target_link_libraries(bp_shm_test GTest::gtest_main dcplib-common dcplib-bp ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(common_tt_test GTest::gtest_main)
target_link_libraries(common_shm_test GTest::gtest_main dcplib-common ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(common_ser_test GTest::gtest_main)
target_link_libraries(common_cs_test GTest::gtest_main dcplib-common ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(common_avl_test GTest::gtest_main dcplib-common)
target_link_libraries(common_misc_test GTest::gtest_main dcplib-common ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(vardis_tt_test GTest::gtest_main dcplib-common dcplib-vardis)
target_link_libraries(vardis_pd_test GTest::gtest_main dcplib-common dcplib-vardis)
include(GoogleTest)
gtest_discover_tests(bp_shm_test)
gtest_discover_tests(common_tt_test)
gtest_discover_tests(common_shm_test)
gtest_discover_tests(common_ser_test)
gtest_discover_tests(common_cs_test)
gtest_discover_tests(common_avl_test)
gtest_discover_tests(common_misc_test)
gtest_discover_tests(vardis_tt_test)
gtest_discover_tests(vardis_pd_test)

