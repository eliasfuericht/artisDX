cmake_minimum_required(VERSION 3.20)
project(artisDX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Automatically find all .cpp files in the src directory
file(GLOB_RECURSE ARTISDX_SOURCES "src/*.cpp")

# Automatically find all .h/.hpp files in the include directory
file(GLOB_RECURSE ARTISDX_HEADERS "include/*.h")

# Specify target executable with all source files
add_executable(artisDX ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

# Enable Precompiled Headers
target_precompile_headers(artisDX PRIVATE include/pch.h)

set(EXTERN_DIR "${CMAKE_SOURCE_DIR}/extern")

# Add ImGui source files
file(GLOB IMGUI_SOURCES 
    "${EXTERN_DIR}/imgui/*.cpp"
    "${EXTERN_DIR}/imgui/backends/imgui_impl_dx12.cpp" 
    "${EXTERN_DIR}/imgui/backends/imgui_impl_win32.cpp"
)

target_sources(artisDX PRIVATE ${IMGUI_SOURCES})

# Add include directories
target_include_directories(artisDX PRIVATE include "${EXTERN_DIR}/imgui" "${EXTERN_DIR}/imgui/backends")

target_link_libraries(
  artisDX     # main application
  d3d12       # DirectX 12 core library
  dxgi        # DirectX Graphics Infrastructure
  dxguid      # DirectX GUID (for interfaces)
  D3DCompiler    # D3DCompiler for shader compilation functions
)

### Organization in VS ###

# Create filters
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "src" FILES ${ARTISDX_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "include" FILES ${ARTISDX_HEADERS})
source_group(TREE "${EXTERN_DIR}/imgui" PREFIX "extern/ImGUI" FILES ${IMGUI_SOURCES})

# Putting precompiled stuff into own filter
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/Debug/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/Release/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/MinSizeRel/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/RelWithDebInfo/cmake_pch.hxx)
source_group("precompiled" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/artisDX.dir/cmake_pch.cxx)

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)