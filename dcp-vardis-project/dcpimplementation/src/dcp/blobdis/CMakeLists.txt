project(DCPStack-BlobDis VERSION 0.1.0 DESCRIPTION "DCP Blob Dissemination Protocol" LANGUAGES CXX)

add_library(dcp-blobdis SHARED
  blobdis_service_primitives.cc
  blobdis_status.cc)


#add_executable (dcpmain-blobdis blobdis_main.cc)

# targets linked against OMNeT++ simulation tool, if available
if (DEFINED ENV{__omnetpp_root_dir})
  set(OPP_INCLUDE_PATH "$ENV{__omnetpp_root_dir}/include")
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\") with OMNeT++ support")
  add_compile_options(-D__DCPSIMULATION__ -I${OPP_INCLUDE_PATH})
  add_library(dcpsim-blobdis SHARED
    blobdis_status.cc
    blobdis_service_primitives.cc)
  target_include_directories(dcpsim-blobdis PUBLIC ${CMAKE_SOURCE_DIR})
else()
  message(STATUS "Configuring subproject ${PROJECT_NAME} (\"${PROJECT_DESCRIPTION}\")")
endif()

target_include_directories(dcp-blobdis PUBLIC ${CMAKE_SOURCE_DIR})
#target_include_directories(dcpmain-blobdis PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(dcp-blobdis PUBLIC dcp-bp dcp-common)
#target_link_libraries(dcpmain-blobdis PRIVATE dcp-blobdis)
