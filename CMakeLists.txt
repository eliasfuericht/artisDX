cmake_minimum_required(VERSION 3.20)
project(artisDX)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "Configuring sources...")

set(APPLICATION_NAME "artisDX")

file(GLOB_RECURSE ARTISDX_SOURCES "src/*.cpp")
file(GLOB_RECURSE ARTISDX_HEADERS "include/*.h")

add_executable(${APPLICATION_NAME} ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

add_subdirectory(extern)

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

# Putting precompiled stuff into own filter
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/Debug/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/Release/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/MinSizeRel/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/RelWithDebInfo/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/cmake_pch.cxx)

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)