cmake_minimum_required(VERSION 3.20)
project(artisDX)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "Configuring sources...")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(APPLICATION_NAME "artisDX")

file(GLOB_RECURSE ARTISDX_SOURCES "src/*.cpp")
file(GLOB_RECURSE ARTISDX_HEADERS "include/*.h")

add_executable(${APPLICATION_NAME} ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

# doesnt work as wanted for some reason
# target_precompile_headers(${APPLICATION_NAME} PRIVATE include/precompiled/pch.h)

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
include(extern/directxtex.cmake)

message(STATUS "External libraries configured successfully.")

# Add include directories
target_include_directories(${APPLICATION_NAME} PRIVATE include)

target_link_libraries(
  ${APPLICATION_NAME}    
  d3d12    
  dxgi        
  dxguid      
  D3DCompiler 
  DearImGui
  fastgltf
  DirectXTex
)

### Organization in VS ###
file(GLOB_RECURSE ARTISDX_SHADERS "shaders/*.fx")
set_source_files_properties(${ARTISDX_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(${APPLICATION_NAME} PRIVATE ${ARTISDX_SHADERS})
source_group(TREE "${CMAKE_SOURCE_DIR}/shaders" PREFIX "shaders" FILES ${ARTISDX_SHADERS})

# Create filters
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "src" FILES ${ARTISDX_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/include" PREFIX "include" FILES ${ARTISDX_HEADERS})

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)