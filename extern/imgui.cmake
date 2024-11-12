CPMAddPackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  GIT_TAG docking
)
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp
     ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
     ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

add_library(DearImGui ${IMGUI_SOURCES})

target_include_directories(DearImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

set_property(TARGET DearImGui PROPERTY FOLDER "extern/ImGUI")