# artisDX - DirectX12 Renderer
<img width="2559" height="1439" alt="project1img1" src="https://github.com/user-attachments/assets/0a2f0221-02ff-4fe4-8c36-aad8ac6a557f" />

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
   
