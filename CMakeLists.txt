cmake_minimum_required(VERSION 3.20)
project(artisDX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Automatically find all .cpp files in the src directory
file(GLOB_RECURSE ARTISDX_SOURCES "src/*.cpp")

# Automatically find all .h/.hpp files in the include directory
file(GLOB_RECURSE ARTISDX_HEADERS "include/*.h")

# Create source groups based on the directory structure
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ARTISDX_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${ARTISDX_HEADERS})

# Specify target executable with all source files
add_executable(artisDX ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

# Add include directories
target_include_directories(artisDX PRIVATE include)

# Enable Precompiled Headers
target_precompile_headers(artisDX PRIVATE include/pch.h)  # Path to your precompiled header

# Link DirectX 12 libraries
target_link_libraries(artisDX
  d3d12       # DirectX 12 core library
  dxgi        # DirectX Graphics Infrastructure
  dxguid      # DirectX GUID (for interfaces)
  D3DCompiler    # D3DCompiler for shader compilation functions
)

# Add ImGui to the project
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/extern/imgui")  # Adjust this to your ImGui directory

# Add ImGui source files
file(GLOB IMGUI_SOURCES 
    "${IMGUI_DIR}/*.cpp" 
    "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp" 
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
)

# Add ImGui include directories
target_include_directories(artisDX PRIVATE "${IMGUI_DIR}" "${IMGUI_DIR}/backends")

# Add ImGui sources to the target executable
target_sources(artisDX PRIVATE ${IMGUI_SOURCES})

# Add required dependencies for ImGui (e.g., DirectX12, Win32 platform)
target_link_libraries(artisDX
  d3d12
  dxgi
  dxguid
  D3DCompiler
)

# Ensure Win32 is defined for ImGui when building on Windows
target_compile_definitions(artisDX PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

# Custom command for precompiled headers
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/CMakeFiles/artisDX.dir/cmake_pch.hxx.pch
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/CMakeFiles/artisDX.dir/cmake_pch.hxx.pch
    DEPENDS ${ARTISDX_SOURCES}  # List the dependencies (your cpp files)
)

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)

# make it a window application - maybe used later
#set_target_properties(artisDX PROPERTIES
#  WIN32_EXECUTABLE TRUE    # This sets the Windows subsystem
#)

# Post-build commands (e.g., copy shaders or assets)
# add_custom_command(TARGET artisDX POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_directory
#  ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:artisDX>/shaders
# )
