# ========================================================================================
# Global settings
# ========================================================================================
cmake_minimum_required(VERSION 3.25.1)
project(DCPStack VERSION 1.3.6 DESCRIPTION "DCP Stack Linux Implementation" LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

add_compile_options(
  # common options
  -Wall
  -Wextra
  -Wpedantic
  -fPIC
  -fopenmp
  -DBOOST_LOG_DYN_LINK
  -I.

  # Debug-specific flags
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Debug>:-g>

  # Release-specific flags
  $<$<CONFIG:Release>:-O3>
)

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

# ========================================================================================

find_package(Boost COMPONENTS program_options log log_setup REQUIRED)
set(PROJECT_LIB ${PROJECT_LIB} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY})
find_library(TINS_LIB tins REQUIRED)
find_library(NCURSES_LIB ncurses REQUIRED)

include(CTest)
enable_testing()


# ========================================================================================
# Sub-projects
# ========================================================================================


add_subdirectory(dcp/common)
add_subdirectory(dcp/bp)
add_subdirectory(dcp/srp)
add_subdirectory(dcp/vardis)
add_subdirectory(dcp/applications)
add_subdirectory(dcp/test)


# ========================================================================================
# Documentation
# ========================================================================================

find_package (Doxygen REQUIRED dot mscgen dia)

doxygen_add_docs(generate_docs
  ${CMAKE_SOURCE_DIR}/dcp/common
  ${CMAKE_SOURCE_DIR}/dcp/bp
  ${CMAKE_SOURCE_DIR}/dcp/srp
  ${CMAKE_SOURCE_DIR}/dcp/vardis
  ALL
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  CONFIG_FILE ${CMAKE_SOURCE_DIR}/doxygen.config
)
