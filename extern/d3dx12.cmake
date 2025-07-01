CPMAddPackage(
  NAME d3dx12
  GITHUB_REPOSITORY microsoft/DirectX-Headers
  GIT_TAG v1.615.0
)

set_property(TARGET DirectX-Guids DirectX-Headers PROPERTY FOLDER "extern/D3DX12")