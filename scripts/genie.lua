print('==> is windows: ' .. tostring(os.is('windows')))
print('==> os.get(): ' .. os.get())
print('==> os.host(): ' .. os.host())

local OF_ROOT = '../'
dofile(path.join(OF_ROOT, 'scripts/utils.lua'))
dofile(path.join(OF_ROOT, 'scripts/toolchain.lua'))
toolchain()

local PROJECT_ROOT = '../'
local buildroot = "./"

if IS_VS then
	buildroot = '../' .. VS_VERSION .. '/'
elseif IS_IOS then
	buildroot = './ios/'
elseif IS_MAC then
	buildroot = './mac/'
elseif IS_ANDROID then
	buildroot = './android/'
end

LIB_KIND = 'StaticLib'

solution "MrEngine"
	location(buildroot)
	configurations { "Debug", "Release" }
	platforms { 'x32', 'x64' }
	startproject "MrApp"
	defines { 'VR_WINDOWS','VR_VULKAN=1','VR_GLES=1','VR_D3D=1','VK_USE_PLATFORM_WIN32_KHR','FILAMENT_DRIVER_SUPPORTS_VULKAN' }

	--单线程模式
	defines { 'UTILS_NO_THREADING=1' }

	-- if IS_VS then flags {"StaticRuntime"} end -- TODO: need to use Dynamic runtime ?
	if IS_VS then
		defines {
			"_CRT_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_DEPRECATE",
			"_CRT_NONSTDC_NO_DEPRECATE",
			"_WINSOCK_DEPRECATED_NO_WARNINGS",
			"WIN32",
			"WIN32_LEAN_AND_MEAN",
		}

		flags { 'MinimumWarnings' }

		buildoptions { '/wd4819', '/EHsc' }
	end

	configuration { "Release" }
		flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "No64BitChecks", "Symbols" }
		defines { "NDEBUG" }
		objdir(buildroot .. "obj/Release")
		targetdir(PROJECT_ROOT .. "bin/Release")
		debugdir(PROJECT_ROOT .. "bin/Release")

	configuration { "Debug" }
		flags { "Symbols" }
		objdir(buildroot .. "obj/Debug")
		targetdir(PROJECT_ROOT .. "bin/Debug")
		debugdir(PROJECT_ROOT .. "bin/Debug")

	configuration {}

------------------------

-- xcodetargetopts {
--     CODE_SIGN_IDENTITY = 'Apple Development',
--     CODE_SIGN_STYLE = 'Automatic',
--     DEVELOPMENT_TEAM = '25F3RKJN6L';
-- }

xcodeprojectopts {
	BUILD_DIR = "$(SRCROOT)/../../build",
	DSTROOT = '$(BUILD_DIR)',
	SYMROOT = '$(BUILD_DIR)',
	OBJROOT = '$(BUILD_DIR)/objects',
	-- CONFIGURATION_BUILD_DIR = "$(BUILD_DIR)/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libs",
	ENABLE_BITCODE = 'YES',
	CLANG_ENABLE_OBJC_ARC = 'YES',
	GCC_GENERATE_DEBUGGING_SYMBOLS = 'YES',
	GCC_INLINES_ARE_PRIVATE_EXTERN = 'YES',
	GCC_SYMBOLS_PRIVATE_EXTERN = 'YES',
	GCC_ENABLE_CPP_RTTI = "YES",
	GCC_ENABLE_CPP_EXCEPTIONS = "YES",
	GCC_ENABLE_CPP_EXCEPTIONS = "YES",
	GCC_C_LANGUAGE_STANDARD = 'c99',
	CLANG_CXX_LANGUAGE_STANDARD = 'c++17',
	CLANG_CXX_LIBRARY = 'libc++',
	-- warning config begin
	CLANG_WARN_DOCUMENTATION_COMMENTS = 'NO',
	CLANG_WARN_COMMA = 'NO',
	GCC_WARN_UNUSED_VARIABLE = 'NO',
	GCC_WARN_64_TO_32_BIT_CONVERSION = 'NO',
	CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = 'NO',
	GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS = 'YES',
	-- warning config end
}

project 'MrEngineDep'
	kind(LIB_KIND)
	language 'C++'
	
	flags { "Cpp11" }

	defines {'ENABLE_HLSL', 'ENABLE_OPT=0'}

	files {
		'../src/MrEngine/Dep/filament/filament/backend/src/noop/NoopDriver.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/noop/PlatformNoop.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/CircularBuffer.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/CommandBufferQueue.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/CommandStream.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/Driver.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/Handle.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/Platform.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/Program.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/SamplerGroup.cpp',
		'../src/MrEngine/Dep/filament/filament/backend/src/TextureReshaper.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/Allocator.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/ashmem.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/CallStack.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/CountDownLatch.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/CString.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/Log.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/ostream.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/Panic.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/Profiler.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/sstream.cpp',
		'../src/MrEngine/Dep/filament/libs/utils/src/Systrace.cpp',
	}

	includedirs {
		'../src/MrEngine/Dep/filament/filament/backend/include',
		'../src/MrEngine/Dep/filament/filament/backend/src',
		'../src/MrEngine/Dep/filament/libs/math/include',
		'../src/MrEngine/Dep/filament/libs/utils/include',
	}

	if IS_VS then
		--filament opengl
		files {
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/gl_headers.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/GLUtils.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/OpenGLBlitter.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/OpenGLDriver.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/OpenGLPlatform.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/OpenGLProgram.cpp',
		}
		includedirs {
			'../src/MrEngine/Dep/filament/libs/bluegl/include',
		}
		--filament vulkan
		files {
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanBinder.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanBuffer.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanContext.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanDisposer.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanDriver.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanFboCache.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanHandles.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanPlatform.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanSamplerCache.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanStagePool.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/VulkanUtility.cpp',
			'../src/MrEngine/Dep/filament/libs/bluevk/src/BlueVK.cpp',
		}
		includedirs {
			'../src/MrEngine/Dep/filament/libs/bluevk/include',
			'../src/MrEngine/Dep/filament/third_party/vkmemalloc/src',
		}
		--filament d3d
		files {
			'../src/MrEngine/Dep/filament/filament/backend/src/d3d/D3D11Context.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/d3d/D3D11Driver.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/d3d/D3D11Handles.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/d3d/PlatformD3D11.cpp',
		}

		files {
			'../src/MrEngine/Dep/filament/filament/backend/src/opengl/PlatformWGL.cpp',
			'../src/MrEngine/Dep/filament/libs/bluegl/src/BlueGL.cpp',
			'../src/MrEngine/Dep/filament/libs/bluegl/src/BlueGLWindows.cpp',
			--'../src/MrEngine/Dep/filament/libs/bluegl/src/BlueGLCoreWindowsImpl.S',
			'../src/MrEngine/Dep/filament/libs/bluegl/src/BlueGLCoreWindows32Impl.cpp',
			'../src/MrEngine/Dep/filament/filament/backend/src/vulkan/PlatformVkWindows.cpp',
			'../src/MrEngine/Dep/filament/libs/bluevk/src/BlueVKWindows.cpp',
		}
		includedirs {
			'../src/MrEngine/Dep/filament/libs/bluevk/include',
		}
      
		--windows
		
		--glslang
		files {
			'../src/MrEngine/Dep/glslang/glslang/CInterface/**.*',
			'../src/MrEngine/Dep/glslang/glslang/GenericCodeGen/**.*',
			'../src/MrEngine/Dep/glslang/glslang/HLSL/**.*',
			'../src/MrEngine/Dep/glslang/glslang/Include/**.*',
			'../src/MrEngine/Dep/glslang/glslang/MachineIndependent/**.*',
			'../src/MrEngine/Dep/glslang/glslang/OSDependent/osinclude.h',
			'../src/MrEngine/Dep/glslang/glslang/OSDependent/Windows/**.*',
			'../src/MrEngine/Dep/glslang/glslang/Public/**.*',
			'../src/MrEngine/Dep/glslang/glslang/ResourceLimits/**.*',
			'../src/MrEngine/Dep/glslang/OGLCompilersDLL/**.*',
			'../src/MrEngine/Dep/glslang/StandAlone/**.*',
			'../src/MrEngine/Dep/glslang/SPIRV/**.*',
		}
		includedirs {
			'../src/MrEngine/Dep/glslang',
			'../src/MrEngine/Dep/glslang/OGLCompilersDLL',
		}

		--spirv-cross
		files {
			'../src/MrEngine/Dep/SPIRV-Cross/**.cpp',
		}
		removefiles {
			'../src/MrEngine/Dep/SPIRV-Cross/samples/**.*',
			'../src/MrEngine/Dep/SPIRV-Cross/tests-other/**.*',
		}
		includedirs {
			'../src/MrEngine/Dep/SPIRV-Cross/include',
		}

	end
		

	targetname 'MrEngineDep'

project 'MrEngine'
	kind(LIB_KIND)
	language 'C++'
	
	flags { "Cpp11" }

	includedirs {
		'../src/MrEngine/Dep/filament/filament/backend/include',
		'../src/MrEngine/Dep/filament/libs/math/include',
		'../src/MrEngine/Dep/filament/libs/utils/include',
		'../src/MrEngine/Dep/filament/libs/bluevk/include',
		'../src/MrEngine/Dep/glslang',
		'../src/MrEngine/Dep/SPIRV-Cross',
	}

	files {
		'../src/MrEngine/Engine/**.*',
	}
    links { "MrEngineDep" }
	targetname 'MrEngine'

project 'MrApp'
	kind 'ConsoleApp'
	language 'C++'
	
	flags { "Cpp11" }

	includedirs {
		'../src/MrEngine/Dep/filament/filament/backend/include',
		'../src/MrEngine/Dep/filament/libs/math/include',
		'../src/MrEngine/Dep/filament/libs/utils/include',
		'../src/MrEngine/Dep/filament/libs/bluevk/include',
		'../src/MrEngine/Engine',
	}

	files {
		'../src/MrEngine/App/Platform/win/Main.cpp',
	}

    links { "MrEngine" }
	if IS_VS then
		links { 'opengl32','d3d11','d3dcompiler','winmm' }
	end

	targetname 'MrApp'

