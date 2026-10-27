# artisDX - DirectX12 Renderer
<img width="2483" height="1333" alt="render-03" src="https://github.com/user-attachments/assets/703c95c6-3e92-4541-a5a4-29e9ea95ba56" />

## Current Features:
- Runtime .glb Model Loading via Windows File Picker
- ImGui GUI exposing functionality and debug information
- Root Signature Creation using Shader Reflection
- Physically Based Rendering (Metallic-Roughness)
- Multiple Directional and Point Lights
- Directional Shadow Mapping with PCF
- Normal Mapping
- [WIP] Shader Hotreloading

## Clone
```git clone https://github.com/eliasfuericht/artisDX.git```

## Building the Project
1. Run the `setup.bat` script in the root directory.
2. Navigate to the `build` folder and open the project in your preferred IDE.

## Requirements:
- Windows SDK 10.0.26100
- DirectXShader Compiler 

### Manual Build
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
   
