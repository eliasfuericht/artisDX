cmake_minimum_required(VERSION 3.20)
project(artisDX)
set(CMAKE_CXX_STANDARD 20)

message(STATUS "Configuring sources...")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(APPLICATION_NAME "artisDX")

set(ARTISDX_HEADERS 
    src/pch.h
    src/AABB.h
    src/Application.h
    src/Camera.h
    src/CommandQueue.h
    src/DescriptorAllocator.h 
    src/GLTFLoader.h
    src/D3D12Core.h
    src/GUI.h
    src/IGUIComponent.h
    src/Material.h
    src/Primitive.h
    src/Mesh.h
    src/Model.h
    src/ModelManager.h
    src/Texture.h
    src/Window.h
    src/DirectionalLight.h
    src/PointLight.h
    src/ModelNode.h
    src/ShaderPass.h 
    src/Shader.h
    src/CommandContext.h
    src/ShadowMap.h
)

set(ARTISDX_SOURCES 
    src/AABB.cpp
    src/Application.cpp
    src/Camera.cpp
    src/CommandQueue.cpp
    src/DescriptorAllocator.cpp
    src/GLTFLoader.cpp
    src/D3D12Core.cpp
    src/GUI.cpp
    src/IGUIComponent.cpp
    src/main.cpp
    src/Material.cpp
    src/Primitive.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/ModelManager.cpp
    src/Texture.cpp
    src/Window.cpp
    src/DirectionalLight.cpp
    src/PointLight.cpp
    src/ModelNode.cpp
    src/ShaderPass.cpp
    src/Shader.cpp
    src/CommandContext.cpp
    src/ShadowMap.cpp
)

add_executable(${APPLICATION_NAME} ${ARTISDX_SOURCES} ${ARTISDX_HEADERS})

target_precompile_headers(${APPLICATION_NAME} PRIVATE src/pch.h)

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
  dxcompiler
  DearImGui
  fastgltf
  DirectXTex
)
 
### Organization in VS ###
file(GLOB_RECURSE ARTISDX_SHADERS "shaders/*.hlsl")
set_source_files_properties(${ARTISDX_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(${APPLICATION_NAME} PRIVATE ${ARTISDX_SHADERS})
source_group(TREE "${CMAKE_SOURCE_DIR}/shaders" PREFIX "shaders" FILES ${ARTISDX_SHADERS})

# Create filters
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "src" FILES ${ARTISDX_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "include" FILES ${ARTISDX_HEADERS})

# hidding pch filters
source_group("precompiled" FILES
  ${CMAKE_BINARY_DIR}/CMakeFiles/${APPLICATION_NAME}.dir/Debug/cmake_pch.hxx
  ${CMAKE_BINARY_DIR}/CMakeFiles/${APPLICATION_NAME}.dir/Release/cmake_pch.hxx 
  ${CMAKE_BINARY_DIR}/CMakeFiles/${APPLICATION_NAME}.dir/MinSizeRel/cmake_pch.hxx
  ${CMAKE_BINARY_DIR}/CMakeFiles/${APPLICATION_NAME}.dir/RelWithDebInfo/cmake_pch.hxx
  ${CMAKE_BINARY_DIR}/CMakeFiles/${APPLICATION_NAME}.dir/cmake_pch.cxx
)

# Set artisDX as the startup project - only works for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT artisDX)