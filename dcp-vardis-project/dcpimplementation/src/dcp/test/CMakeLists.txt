project(DCPStack-Tests VERSION 1.3.6 DESCRIPTION "DCP Stack Test Suite" LANGUAGES CXX)

message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")

# -------------

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

# -------------

add_executable(bp_shm_test "bp/bp_client_sharedmem_test.cc")
add_executable(common_tt_test "common/transmissible_types_test.cc")
add_executable(common_shm_test "common/shared_mem_area_test.cc")
add_executable(common_ser_test "common/serialization_area_test.cc")
add_executable(common_cs_test "common/command_socket_test.cc")
add_executable(common_avl_test "common/avl_tree.cc")
add_executable(common_misc_test "common/miscellaneous_test.cc")
add_executable(vardis_tt_test "vardis/vardis_transmissible_types_test.cc")
add_executable(vardis_pd_test "vardis/vardis_protocol_data_test.cc")

target_include_directories(bp_shm_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_tt_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_shm_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_ser_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_cs_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_avl_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(common_misc_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(vardis_tt_test PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(vardis_pd_test PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(bp_shm_test PRIVATE GTest::gtest_main dcp-common dcp-bp)
target_link_libraries(common_tt_test PRIVATE GTest::gtest_main)
target_link_libraries(common_shm_test PRIVATE GTest::gtest_main dcp-common)
target_link_libraries(common_ser_test PRIVATE GTest::gtest_main)
target_link_libraries(common_cs_test PRIVATE GTest::gtest_main dcp-common)
target_link_libraries(common_avl_test PRIVATE GTest::gtest_main dcp-common)
target_link_libraries(common_misc_test PRIVATE GTest::gtest_main dcp-common)
target_link_libraries(vardis_tt_test PRIVATE GTest::gtest_main dcp-common dcp-vardis)
target_link_libraries(vardis_pd_test PRIVATE GTest::gtest_main dcp-common dcp-vardis)

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

