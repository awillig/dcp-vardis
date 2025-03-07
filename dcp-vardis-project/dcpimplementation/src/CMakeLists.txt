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


add_library(dcpcommon SHARED ${common_SRC})
add_library(dcpbp SHARED ${bp_SRC})
add_library(dcpsrp SHARED ${srp_SRC})
add_library(dcpvardis SHARED ${vardis_SRC})
add_executable(dcp-bp "dcp/main/bp_main.cc")
add_executable(dcp-srp "dcp/main/srp_main.cc")
add_executable(dcp-vardis "dcp/main/vardis_main.cc")
add_executable(vardisapp-test-producer "dcp/applications/vardisapp-test-producer.cc")
add_executable(vardisapp-test-consumer "dcp/applications/vardisapp-test-consumer.cc")
add_executable(vardisapp-delete-variable "dcp/applications/vardisapp-delete-variable.cc")
add_executable(vardisapp-describe-variable "dcp/applications/vardisapp-describe-variable.cc")
add_executable(vardisapp-list-variables "dcp/applications/vardisapp-list-variables.cc")
add_executable(srpapp-test-generate-sd "dcp/applications/srpapp-test-generate-sd.cc")
add_executable(srpapp-display-neighbour-table "dcp/applications/srpapp-display-neighbour-table.cc")

find_package(Boost COMPONENTS program_options log log_setup REQUIRED)
set(PROJECT_LIB ${PROJECT_LIB} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
find_library(TINS_LIB tins)

target_link_libraries(dcpbp dcpcommon tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(dcpsrp dcpbp)
target_link_libraries(dcpvardis dcpbp)
target_link_libraries(dcp-bp -Wl,--start-group  ${PROJECT_LIB} tins dcpcommon dcpbp -Wl,--end-group)
target_link_libraries(dcp-srp -Wl,--start-group tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpbp dcpsrp -Wl,--end-group)
target_link_libraries(dcp-vardis -Wl,--start-group tins ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpbp dcpvardis -Wl,--end-group)
target_link_libraries(vardisapp-test-producer -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpvardis -Wl,--end-group)
target_link_libraries(vardisapp-test-consumer -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpvardis ncurses -Wl,--end-group)
target_link_libraries(vardisapp-delete-variable -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpvardis -Wl,--end-group)
target_link_libraries(vardisapp-describe-variable -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpvardis -Wl,--end-group)
target_link_libraries(vardisapp-list-variables -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpvardis -Wl,--end-group)
target_link_libraries(srpapp-test-generate-sd -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpsrp -Wl,--end-group)
target_link_libraries(srpapp-display-neighbour-table -Wl,--start-group ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} dcpcommon dcpsrp -Wl,--end-group)


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


add_executable(common_tt_test "test/common/transmissible_types_test.cc")
add_executable(common_shm_test "test/common/shared_mem_area_test.cc")
add_executable(common_ser_test "test/common/serialization_area_test.cc")
add_executable(common_cs_test "test/common/command_socket_test.cc")
add_executable(common_avl_test "test/common/avl_tree.cc")
add_executable(vardis_tt_test "test/vardis/vardis_transmissible_types_test.cc")
add_executable(vardis_pd_test "test/vardis/vardis_protocol_data_test.cc")
target_link_libraries(common_tt_test GTest::gtest_main)
target_link_libraries(common_shm_test GTest::gtest_main dcpcommon ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(common_ser_test GTest::gtest_main)
target_link_libraries(common_cs_test GTest::gtest_main dcpcommon ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
target_link_libraries(common_avl_test GTest::gtest_main dcpcommon)
target_link_libraries(vardis_tt_test GTest::gtest_main dcpcommon dcpvardis)
target_link_libraries(vardis_pd_test GTest::gtest_main dcpcommon dcpvardis)
include(GoogleTest)
gtest_discover_tests(common_tt_test)
gtest_discover_tests(common_shm_test)
gtest_discover_tests(common_ser_test)
gtest_discover_tests(common_cs_test)
gtest_discover_tests(common_avl_test)
gtest_discover_tests(vardis_tt_test)
gtest_discover_tests(vardis_pd_test)

