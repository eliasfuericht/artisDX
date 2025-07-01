CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.4
)

set_property(TARGET glfw uninstall update_mappings PROPERTY FOLDER "extern/GLFW")