message(STATUS "Configuring external libraries...")

if(NOT EXISTS ${CMAKE_BINARY_DIR}/cmake/CPM.cmake)
  file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
       ${CMAKE_BINARY_DIR}/cmake/CPM.cmake
  )
endif()

include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

include(imgui.cmake)
include(fastgltf.cmake)
include(d3dx12.cmake)

message(STATUS "External libraries configured successfully.")