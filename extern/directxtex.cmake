CPMAddPackage(
  NAME directxtex
  GITHUB_REPOSITORY microsoft/DirectXTex
  GIT_TAG mar2025
)

set_property(TARGET DirectXTex ddsview texassemble texconv texdiag PROPERTY FOLDER "extern/DirectXTex")