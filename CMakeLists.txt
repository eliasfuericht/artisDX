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

# make it a window application - maybe used later
#set_target_properties(artisDX PROPERTIES
#  WIN32_EXECUTABLE TRUE    # This sets the Windows subsystem
#)

# Add additional dependencies (e.g., DirectXTK or DirectXMath if needed)
# find_package(DirectXTK REQUIRED)
# target_link_libraries(artisDX DirectXTK)

########### later ###########
# Post-build commands (e.g., copy shaders or assets)
# add_custom_command(TARGET artisDX POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_directory
#  ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:artisDX>/shaders
# )
