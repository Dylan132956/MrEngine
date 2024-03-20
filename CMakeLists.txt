cmake_minimum_required(VERSION 3.7.2)

project(Project)

get_filename_component(MR3D_LIB_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/src/MrEngine
                       ABSOLUTE)

get_filename_component(MR3D_APP_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/src/MrEngine/App
                       ABSOLUTE)

function(string_ends_with str suffix var)
    string(LENGTH "${str}" str_length)
    string(LENGTH "${suffix}" suffix_length)
    set(value FALSE)
    if (NOT ${str_length} LESS ${suffix_length})
        math(EXPR str_offset "${str_length} - ${suffix_length}")
        string(SUBSTRING "${str}" ${str_offset} ${suffix_length} str_suffix)
        if (str_suffix STREQUAL suffix)
            set(value TRUE)
        endif ()
    endif ()
    set(${var} ${value} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE MR3D_ASSETS ${MR3D_APP_SRC_DIR}/bin/Assets/*.*)
foreach (item ${MR3D_ASSETS})
    string_ends_with(${item} ".cache" is_cache)
    if (${is_cache})
        list(REMOVE_ITEM MR3D_ASSETS ${item})
    endif ()
endforeach ()

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -DFT2_BUILD_LIBRARY -DAL_LIBTYPE_STATIC -DAL_ALEXT_PROTOTYPES -DFPM_DEFAULT -DSIZEOF_INT=4 -DENABLE_OPT=0 -DENABLE_HLSL")

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DFT2_BUILD_LIBRARY -DAL_LIBTYPE_STATIC -DAL_ALEXT_PROTOTYPES -DFPM_DEFAULT -DSIZEOF_INT=4 -DENABLE_OPT=0 -DENABLE_HLSL")

file(GLOB MR3D_DEP_SRCS
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/noop/NoopDriver.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/noop/PlatformNoop.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/CircularBuffer.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/CommandBufferQueue.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/CommandStream.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/Driver.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/Handle.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/Platform.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/Program.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/SamplerGroup.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/TextureReshaper.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/Allocator.cpp
     ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/ashmem.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/CallStack.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/CountDownLatch.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/CString.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/Log.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/ostream.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/Panic.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/Profiler.cpp
	 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/sstream.cpp
     ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/Systrace.cpp
     )

source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS})

add_library(Mr3DDep STATIC ${MR3D_DEP_SRCS})

target_include_directories(Mr3DDep PRIVATE
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/include
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/math/include
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/include
                           )

file(GLOB MR3D_LIB_SRCS
     ${MR3D_LIB_SRC_DIR}/Engine/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/animation/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/animation/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/graphics/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/graphics/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/io/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/io/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/math/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/math/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/memory/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/memory/*.cpp
	 ${MR3D_LIB_SRC_DIR}/Engine/postprocessing/*.h
	 ${MR3D_LIB_SRC_DIR}/Engine/postprocessing/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/string/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/string/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/time/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/time/*.cpp
     ${MR3D_LIB_SRC_DIR}/Engine/jpeg/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/jpeg/*.c
     ${MR3D_LIB_SRC_DIR}/Engine/png/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/png/*.c
     ${MR3D_LIB_SRC_DIR}/Engine/zlib/*.h
     ${MR3D_LIB_SRC_DIR}/Engine/zlib/*.c
     )

source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_LIB_SRCS})

add_library(Mr3D STATIC ${MR3D_LIB_SRCS})

target_include_directories(Mr3D PRIVATE
                           ${MR3D_LIB_SRC_DIR}/Engine
                           ${MR3D_LIB_SRC_DIR}/Engine/zlib
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/include
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/math/include
						   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/include
                           ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/include
                           ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang
                           ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/SPIRV-Cross
                           ${MR3D_LIB_SRC_DIR}/Dep/tinygltf
                           ${MR3D_LIB_SRC_DIR}/Dep/stb
                           ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter
                           )

file(GLOB MR3D_APP_SRCS
     ${MR3D_APP_SRC_DIR}/App.cpp
     ${MR3D_APP_SRC_DIR}/AppImplementGLTF.h
     )
source_group(TREE ${MR3D_APP_SRC_DIR}/.. FILES ${MR3D_APP_SRCS})
# filament opengl
if (${Target} MATCHES "Windows" OR
    ${Target} MATCHES "Android" OR
	${Target} MATCHES "Mac" OR
    ${Target} MATCHES "iOS" OR
	${Target} MATCHES "WASM")

    file(GLOB MR3D_DEP_SRCS_FILAMENT_OPENGL
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/gl_headers.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/GLUtils.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/OpenGLBlitter.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/OpenGLDriver.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/OpenGLPlatform.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/OpenGLProgram.cpp
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_FILAMENT_OPENGL})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_FILAMENT_OPENGL})

    target_include_directories(Mr3DDep PRIVATE
                               ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/include
                               )

endif ()

# filament vulkan
if (${Target} MATCHES "Windows" OR
    ${Target} MATCHES "Android")

    file(GLOB MR3D_DEP_SRCS_FILAMENT_VULKAN
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanBinder.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanBuffer.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanContext.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanDisposer.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanDriver.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanFboCache.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanHandles.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanPlatform.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanSamplerCache.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanStagePool.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/VulkanUtility.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/src/BlueVK.cpp
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_FILAMENT_VULKAN})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_FILAMENT_VULKAN})

    target_include_directories(Mr3DDep PRIVATE
                               ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/include
							   ${MR3D_LIB_SRC_DIR}/Dep/filament/third_party/vkmemalloc/src
                               )

endif ()

# filament metal
if (${Target} MATCHES "Mac" OR
    ${Target} MATCHES "iOS")

    file(GLOB MR3D_DEP_SRCS_FILAMENT_METAL
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalBlitter.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalBufferPool.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalContext.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalDriver.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalExternalImage.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalHandles.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalResourceTracker.cpp
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/MetalState.mm
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/metal/PlatformMetal.mm
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_FILAMENT_METAL})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_FILAMENT_METAL})

endif ()

# filament d3d
if (${Target} MATCHES "Windows" OR
    ${Target} MATCHES "UWP")

    file(GLOB MR3D_DEP_SRCS_FILAMENT_D3D11
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/d3d/D3D11Context.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/d3d/D3D11Driver.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/d3d/D3D11Handles.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/d3d/PlatformD3D11.cpp
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_FILAMENT_D3D11})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_FILAMENT_D3D11})

endif ()

# glslang
file(GLOB MR3D_DEP_SRCS_GLSLANG
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/CInterface/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/GenericCodeGen/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/HLSL/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/HLSL/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/Include/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/MachineIndependent/preprocessor/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/MachineIndependent/preprocessor/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/MachineIndependent/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/MachineIndependent/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/OSDependent/osinclude.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/Public/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/ResourceLimits/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/OGLCompilersDLL/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/OGLCompilersDLL/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/StandAlone/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/StandAlone/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/SPIRV/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/SPIRV/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/SPIRV/CInterface/*.cpp
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/SPIRV-Cross/*.h
        ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/SPIRV-Cross/*.cpp
        )

source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_GLSLANG})

target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_GLSLANG})

target_include_directories(Mr3DDep PRIVATE
                            ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang
                            ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/OGLCompilersDLL)

# glslang osdep
if (${Target} MATCHES "Windows" )
    file(GLOB MR3D_DEP_SRCS_GLSLANG_OSDEP
            ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/OSDependent/Windows/*.cpp
            )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_GLSLANG_OSDEP})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_GLSLANG_OSDEP})
else()
    file(GLOB MR3D_DEP_SRCS_GLSLANG_OSDEP
            ${MR3D_LIB_SRC_DIR}/Dep/shader-Converter/glslang/glslang/OSDependent/Unix/*.cpp
            )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_GLSLANG_OSDEP})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_GLSLANG_OSDEP})
endif()

if (${Target} MATCHES "Windows")

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -DVR_WINDOWS -DVR_VULKAN=1 -DVR_GLES=1 -DVR_D3D=1 -DVK_USE_PLATFORM_WIN32_KHR -DFILAMENT_DRIVER_SUPPORTS_VULKAN")
    add_definitions(-W3 -D_CRT_SECURE_NO_WARNINGS -D_CRT_RAND_S -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES -DGLEW_STATIC -DENABLE_HLSL)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

    file(GLOB MR3D_DEP_SRCS_WIN
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/PlatformWGL.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGL.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLWindows.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLCoreWindows32Impl.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/PlatformVkWindows.cpp
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/src/BlueVKWindows.cpp
         )

	enable_language(ASM_MASM)
    set_property(SOURCE ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLCoreWindowsImpl.S PROPERTY LANGUAGE ASM_MASM)

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_WIN})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_WIN})

    target_include_directories(Mr3D PRIVATE
							   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/include
                               )

    source_group(src FILES ${MR3D_APP_SRC_DIR}/Platform/win/Main.cpp)

    add_executable(Mr3DApp
                   ${MR3D_APP_SRCS}
                   ${MR3D_APP_SRC_DIR}/Platform/win/Main.cpp
                   )

    target_link_libraries(Mr3DApp
                          Mr3D Mr3DDep
                          opengl32.lib
						  d3d11.lib
						  d3dcompiler.lib
                          winmm.lib
                          Xaudio2.lib
                          )

    set_property(TARGET Mr3DApp PROPERTY LINK_FLAGS "/SUBSYSTEM:CONSOLE")

    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
    string(REPLACE "/" "\\" BIN_DIR ${EXECUTABLE_OUTPUT_PATH}/$(Configuration))
    string(REPLACE "/" "\\" ASSETS_COPY_SRC ${MR3D_APP_SRC_DIR}/bin/Assets)
    string(REPLACE "/" "\\" ICON_SRC ${MR3D_APP_SRC_DIR}/bin/icon.ico)

    add_custom_command(TARGET Mr3DApp
                       POST_BUILD
                       COMMAND xcopy ${ASSETS_COPY_SRC} ${BIN_DIR}\\Assets\\ /s /d /y
                       COMMAND copy /Y ${ICON_SRC} ${BIN_DIR}\\icon.ico
                       )
elseif (${Target} MATCHES "UWP")

elseif (${Target} MATCHES "Android")

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++14 -fexceptions -frtti -DVR_ANDROID -DVR_VULKAN=1 -DVR_GLES=1 -DVK_USE_PLATFORM_ANDROID_KHR -DFILAMENT_DRIVER_SUPPORTS_VULKAN")
    add_definitions(-Wall -DIOAPI_NO_64 -DHAVE_GCC_DESTRUCTOR)

    target_sources(Mr3DDep PRIVATE
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/android/ExternalStreamManagerAndroid.cpp
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/android/ExternalTextureManagerAndroid.cpp
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/linux/Condition.cpp
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/src/linux/Mutex.cpp
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/android/VirtualMachineEnv.cpp
                   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/PlatformEGL.cpp
				   ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/vulkan/PlatformVkAndroid.cpp
				   ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluevk/src/BlueVKLinuxAndroid.cpp
                   )

    add_library(Mr3DApp SHARED
                ${MR3D_APP_SRCS}
                ${CMAKE_SOURCE_DIR}/src/MrEngine/App/Platform/android/app/src/main/jni/jni.cpp
                )

    target_link_libraries(Mr3DApp
                          Mr3D Mr3DDep
                          android log z OpenSLES EGL GLESv3
                          )

elseif (${Target} MATCHES "Mac")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++14 -DVR_MAC -DVR_VULKAN=0 -DVR_GLES=1 -DVR_METAL=1 -DVR_USE_METAL=1")
    add_definitions(-Wall -DHAVE_ASSERT_H -DFILAMENT_SUPPORTS_METAL)

    file(GLOB MR3D_DEP_SRCS_MAC
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/PlatformCocoaGL.mm
		 ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGL.cpp
         ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLDarwin.cpp
         ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLCoreDarwinImpl.S
         )

    enable_language(ASM_MASM)
    set_property(SOURCE ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLCoreDarwinImpl.S PROPERTY LANGUAGE ASM_MASM)
    set_source_files_properties(${MR3D_LIB_SRC_DIR}/Dep/filament/libs/bluegl/src/BlueGLCoreDarwinImpl.S PROPERTIES XCODE_EXPLICIT_FILE_TYPE "sourcecode.asm")

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_MAC})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_MAC})

    set_source_files_properties(${MR3D_LIB_SRC_DIR}/Engine/Debug.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(${MR3D_LIB_SRC_DIR}/Engine/Engine.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")

    set_source_files_properties(${MR3D_APP_SRC_DIR}/Platform/mac/app/Assets.xcassets
                                ${MR3D_APP_SRC_DIR}/bin/Assets
                                PROPERTIES
                                MACOSX_PACKAGE_LOCATION Resources
                                )

    add_executable(Mr3DApp
                   ${MR3D_APP_SRCS}
                   ${MR3D_APP_SRC_DIR}/Platform/mac/Main.m
                   ${MR3D_APP_SRC_DIR}/Platform/mac/AppDelegate.m
                   ${MR3D_APP_SRC_DIR}/Platform/mac/ViewController.mm
                   ${MR3D_APP_SRC_DIR}/Platform/mac/app/Assets.xcassets
                   ${MR3D_APP_SRC_DIR}/bin/Assets
                   )

    target_link_libraries(Mr3DApp
                          Mr3D Mr3DDep
                          "-framework AppKit"
                          "-framework Metal"
                          "-framework QuartzCore"
                          "-framework IOKit"
                          z.1.2.8
                          )

    set_target_properties(Mr3DApp PROPERTIES
                          MACOSX_BUNDLE TRUE
                          MACOSX_BUNDLE_INFO_PLIST ${VIRY3D_APP_SRC_DIR}/Platform/mac/app/Info.plist
                          XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
                          )

elseif (${Target} MATCHES "iOS")

    set(XCODE_IOS_PLATFORM iphoneos)
    execute_process(COMMAND xcodebuild -version -sdk ${XCODE_IOS_PLATFORM} Path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "found ios sdk: ${CMAKE_OSX_SYSROOT}")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "")
    set(CMAKE_OSX_ARCHITECTURES arm64)

    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++14 -DVR_IOS -DVR_VULKAN=0 -DVR_GLES=1 -DVR_METAL=1 -DVR_USE_METAL=0")
    add_definitions(-Wall -DIOS -DHAVE_ASSERT_H -DFILAMENT_SUPPORTS_METAL)

    file(GLOB MR3D_DEP_SRCS_IOS
         ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/src/opengl/PlatformCocoaTouchGL.mm
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_DEP_SRCS_IOS})

    target_sources(Mr3DDep PRIVATE ${MR3D_DEP_SRCS_IOS})

    set_source_files_properties(${MR3D_LIB_SRC_DIR}/Engine/Debug.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(${MR3D_LIB_SRC_DIR}/Engine/Engine.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")

    file(GLOB MR3D_LIB_SRCS_IOS
         ${MR3D_LIB_SRC_DIR}/App/Platform/ios/*.h
         ${MR3D_LIB_SRC_DIR}/App/Platform/ios/*.mm
         )

    source_group(TREE ${MR3D_LIB_SRC_DIR}/.. FILES ${MR3D_LIB_SRCS_IOS})

    target_sources(MR3D PRIVATE ${MR3D_LIB_SRCS_IOS})

    target_include_directories(Mr3D PRIVATE
                               )

    set_source_files_properties(${MR3D_APP_SRC_DIR}/Platform/ios/app/Assets.xcassets
                                ${MR3D_APP_SRC_DIR}/Platform/ios/app/Base.lproj/LaunchScreen.storyboard
                                ${MR3D_APP_SRC_DIR}/bin/Assets
                                PROPERTIES
                                MACOSX_PACKAGE_LOCATION Resources
                                )

    add_executable(Mr3DApp
                   ${MR3D_APP_SRCS}
                   ${MR3D_APP_SRC_DIR}/Platform/ios/Main.m
                   ${MR3D_APP_SRC_DIR}/Platform/ios/AppDelegate.m
                   ${MR3D_APP_SRC_DIR}/Platform/ios/ViewController.mm
                   ${MR3D_APP_SRC_DIR}/Platform/ios/app/Assets.xcassets
                   ${MR3D_APP_SRC_DIR}/Platform/ios/app/Base.lproj/LaunchScreen.storyboard
                   ${MR3D_APP_SRC_DIR}/bin/Assets
                   )

    target_link_libraries(Mr3DApp
                          Mr3D Mr3DDep
                          "-framework UIKit"
                          "-framework Metal"
                          "-framework OpenGLES"
                          "-framework QuartzCore"
                          "-framework IOKit"
                          "-framework ARKit"
                          "-framework CoreVideo"
                          z.1.2.8
                          )

    set_target_properties(Mr3DApp PROPERTIES
                          MACOSX_BUNDLE TRUE
                          MACOSX_BUNDLE_INFO_PLIST ${VIRY3D_APP_SRC_DIR}/Platform/ios/app/Info.plist
                          XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
                          )

elseif (${Target} MATCHES "WASM")

endif ()

target_include_directories(Mr3DApp PRIVATE
                           ${MR3D_LIB_SRC_DIR}/Engine
                           ${MR3D_LIB_SRC_DIR}/Dep/filament/filament/backend/include
                           ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/math/include
                           ${MR3D_LIB_SRC_DIR}/Dep/filament/libs/utils/include
                           ${MR3D_LIB_SRC_DIR}
                           )
