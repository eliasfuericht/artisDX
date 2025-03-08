cmake_minimum_required(VERSION 3.20)
project(artisDX)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "Configuring sources...")

set(APPLICATION_NAME "artisDX")

file(GLOB_RECURSE ARTISDX_SOURCES "src/*.cpp")
file(GLOB_RECURSE ARTISDX_HEADERS "include/*.h")

add_executable(${APPLICATION_NAME} ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

message(STATUS "Configuring external libraries...")

if(NOT EXISTS ${CMAKE_BINARY_DIR}/cmake/CPM.cmake)
  file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
       ${CMAKE_BINARY_DIR}/cmake/CPM.cmake
  )
endif()

include(${CMAKE_BINARY_DIR}/cmake/CPM.cmake)

include(extern/glm.cmake)
include(extern/imgui.cmake)
include(extern/fastgltf.cmake)
include(extern/d3dx12.cmake)

message(STATUS "External libraries configured successfully.")

# Add include directories
target_include_directories(${APPLICATION_NAME} PRIVATE include)

target_link_libraries(
  ${APPLICATION_NAME}     # main application
  d3d12       # DirectX 12 core library
  dxgi        # DirectX Graphics Infrastructure
  dxguid      # DirectX GUID (for interfaces)
  D3DCompiler    # D3DCompiler for shader compilation functions
  DearImGui
  fastgltf
)

### Organization in VS ###

# Create filters
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "src" FILES ${ARTISDX_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "include" FILES ${ARTISDX_HEADERS})

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)