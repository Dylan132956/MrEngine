# MrEngine

Cross-platform 3D Rendering Engine

Support Android、iOS、macOS、Windows

Support OpenGL ES 3.0、Vulkan、Metal、DX11

Shader Language: HLSL

Support HLSL convert to Spirv Shader、GLSL、Metal Shader

Support Shader Parse System: Render State、Muti-Pass

Support Shader Reflection: Uniform Buffer、Samplers

Support C++ Reflection(https://github.com/BoomingTech/Piccolo)

Support Rendering frame、Imgui Rendering、OpenAl Audio Player(https://github.com/stackos/Viry3D)

## Build
### Windows
* Visual Studio 2019
* `gen_build_win_x86.bat` generate project in `build/win_x86`
* `gen_build_win_x64.bat` generate project in `build/win_x64`

### iOS
* Xcode
* `gen_build_ios.sh` generate project in `build`

### macOS
* Xcode
* `gen_build_mac.sh` generate project in `build`

### Android
* Android Studio
* `src/MrEngine/App/Platform/android`

