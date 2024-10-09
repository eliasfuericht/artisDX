cmake_minimum_required(VERSION 3.15)
project(artisDX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify target executables and source files
add_executable(artisDX
  src/main.cpp
)

# Add include directories
target_include_directories(artisDX PRIVATE include)

# Link DirectX 12 libraries
target_link_libraries(artisDX
  d3d12       # DirectX 12 core library
  dxgi        # DirectX Graphics Infrastructure
  dxguid      # DirectX GUID (for interfaces)
)

# Add additional dependencies (e.g., DirectXTK or DirectXMath if needed)
# find_package(DirectXTK REQUIRED)
# target_link_libraries(artisDX DirectXTK)

########### later ###########
# Post-build commands (e.g., copy shaders or assets)
# add_custom_command(TARGET artisDX POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_directory
#  ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:artisDX>/shaders
# )
