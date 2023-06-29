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

project "lua"
    kind "StaticLib"
    language "C++" 
	defines { 'LUA_USE_C89', 'OF_LUA_CPP' }
	local ofLuaSourceFiles = {
		'../src/MrEngine/Dep/lua/*.h*',
		'../src/MrEngine/Dep/lua/*.cpp',
	  }
    files(ofLuaSourceFiles)
    removefiles{
        '../src/MrEngine/Dep/lua/lua.cpp',
        '../src/MrEngine/Dep/lua/luac.cpp',
        '../src/MrEngine/Dep/lua/luaall.cpp',
    }
    if IS_XCODE then 
        configuration { "Release" }
            flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "Symbols" } 
    end
    targetname "lua"

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

project "mcpp"
	kind "StaticLib"
	language "C++"

	defines {
		'_MBCS',
		'MCPP_LIB=1',
	}

	files{
		'../src/MrEngine/Dep/mcpp-2.7.2/src/*.c*',
		'../src/MrEngine/Dep/mcpp-2.7.2/src/*.h',
	}

	removefiles{
		'../src/MrEngine/Dep/mcpp-2.7.2/src/testmain.c'
	}

	targetname "mcpp"

project "hlslcc_lib"
	kind "StaticLib"
	language "C++"

	files{
		'../src/MrEngine/Dep/hlslcc_lib/*.cpp*',
		'../src/MrEngine/Dep/hlslcc_lib/*.h',
	}

	targetname "hlslcc_lib"

--- zilib
project "zlib"
    kind "StaticLib"
    language "C++"
    if IS_VS then
        files{
            '../src/MrEngine/Dep/zlib/*.c*',
            '../src/MrEngine/Dep/zlib/*.h',
        }
	end

    files{
        '../src/MrEngine/Dep/zlib/wrapper/*.c',
        '../src/MrEngine/Dep/zlib/wrapper/*.h',
        '../src/MrEngine/Dep/zlib/wrapper/*.cpp',
        '../src/MrEngine/Dep/zlib/wrapper/*.h',
    }

    includedirs {
        '../src/MrEngine/Dep/zlib',
    }

    exportedIncludedirs_zlib = {
        '../src/MrEngine/Dep/zlib/wrapper'
    }

    if IS_VS then
        table.insert(exportedIncludedirs_zlib, '../src/MrEngine/Dep/zlib')
    end

    if IS_XCODE then 
        configuration { "Release" }
            flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "Symbols" } 
    end
    targetname "zlib"

project "liblz4"
    kind "StaticLib"
    language "C++"

    files{
        '../src/MrEngine/Dep/liblz4/include/lz4.h',
        '../src/MrEngine/Dep/liblz4/include/lz4frame.h',
        '../src/MrEngine/Dep/liblz4/include/lz4frame_static.h',
        '../src/MrEngine/Dep/liblz4/include/lz4hc.h',
        '../src/MrEngine/Dep/liblz4/include/xxhash.h',
        '../src/MrEngine/Dep/liblz4/src/lz4frame.c',
        '../src/MrEngine/Dep/liblz4/src/lz4.c',
        '../src/MrEngine/Dep/liblz4/src/lz4hc.c',
        '../src/MrEngine/Dep/liblz4/src/xxhash.c',
    }

    includedirs {
        '../src/MrEngine/Dep/liblz4/include',
    }

    if IS_XCODE then 
        configuration { "Release" }
            flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "Symbols" } 
    end
    targetname "liblz4"

project "libzip"
    kind "StaticLib"
    language "C++"

    if IS_VS then
        defines {
            'ZIP_STATIC',
        }
    else
        defines {
            'HAVE_UNISTD_H',
            'HAVE_STRINGS_H',
            'HAVE_STRCASECMP',
            'HAVE_FSEEKO',
            'HAVE_FTELLO',
            'HAVE_STRICMP',
        }
    end

    files{
        '../src/MrEngine/Dep/libzip/source/zip_add.c',
        '../src/MrEngine/Dep/libzip/source/zip_add_dir.c',
        '../src/MrEngine/Dep/libzip/source/zip_add_entry.c',
        '../src/MrEngine/Dep/libzip/source/zip_algorithm_deflate.c',
        '../src/MrEngine/Dep/libzip/source/zip_buffer.c',
        '../src/MrEngine/Dep/libzip/source/zip_close.c',
        '../src/MrEngine/Dep/libzip/source/zip_delete.c',
        '../src/MrEngine/Dep/libzip/source/zip_dir_add.c',
        '../src/MrEngine/Dep/libzip/source/zip_dirent.c',
        '../src/MrEngine/Dep/libzip/source/zip_discard.c',
        '../src/MrEngine/Dep/libzip/source/zip_entry.c',
        '../src/MrEngine/Dep/libzip/source/zip_err_str.c',
        '../src/MrEngine/Dep/libzip/source/zip_error.c',
        '../src/MrEngine/Dep/libzip/source/zip_error_clear.c',
        '../src/MrEngine/Dep/libzip/source/zip_error_get.c',
        '../src/MrEngine/Dep/libzip/source/zip_error_get_sys_type.c',
        '../src/MrEngine/Dep/libzip/source/zip_error_strerror.c',
        '../src/MrEngine/Dep/libzip/source/zip_error_to_str.c',
        '../src/MrEngine/Dep/libzip/source/zip_extra_field.c',
        '../src/MrEngine/Dep/libzip/source/zip_extra_field_api.c',
        '../src/MrEngine/Dep/libzip/source/zip_fclose.c',
        '../src/MrEngine/Dep/libzip/source/zip_fdopen.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_add.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_error_clear.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_error_get.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_get_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_get_external_attributes.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_get_offset.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_rename.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_replace.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_set_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_set_encryption.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_set_external_attributes.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_set_mtime.c',
        '../src/MrEngine/Dep/libzip/source/zip_file_strerror.c',
        '../src/MrEngine/Dep/libzip/source/zip_fopen.c',
        '../src/MrEngine/Dep/libzip/source/zip_fopen_encrypted.c',
        '../src/MrEngine/Dep/libzip/source/zip_fopen_index.c',
        '../src/MrEngine/Dep/libzip/source/zip_fopen_index_encrypted.c',
        '../src/MrEngine/Dep/libzip/source/zip_fread.c',
        '../src/MrEngine/Dep/libzip/source/zip_fseek.c',
        '../src/MrEngine/Dep/libzip/source/zip_ftell.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_archive_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_archive_flag.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_encryption_implementation.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_file_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_name.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_num_entries.c',
        '../src/MrEngine/Dep/libzip/source/zip_get_num_files.c',
        '../src/MrEngine/Dep/libzip/source/zip_hash.c',
        '../src/MrEngine/Dep/libzip/source/zip_io_util.c',
        '../src/MrEngine/Dep/libzip/source/zip_libzip_version.c',
        '../src/MrEngine/Dep/libzip/source/zip_memdup.c',
        '../src/MrEngine/Dep/libzip/source/zip_name_locate.c',
        '../src/MrEngine/Dep/libzip/source/zip_new.c',
        '../src/MrEngine/Dep/libzip/source/zip_open.c',
        '../src/MrEngine/Dep/libzip/source/zip_pkware.c',
        '../src/MrEngine/Dep/libzip/source/zip_progress.c',
        '../src/MrEngine/Dep/libzip/source/zip_rename.c',
        '../src/MrEngine/Dep/libzip/source/zip_replace.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_archive_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_archive_flag.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_default_password.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_file_comment.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_file_compression.c',
        '../src/MrEngine/Dep/libzip/source/zip_set_name.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_accept_empty.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_begin_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_begin_write_cloning.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_buffer.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_call.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_close.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_commit_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_compress.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_crc.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_error.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_file_common.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_file_stdio.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_free.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_function.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_get_file_attributes.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_is_deleted.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_layered.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_open.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_pkware_decode.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_pkware_encode.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_read.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_remove.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_rollback_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_seek.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_seek_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_stat.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_supports.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_tell.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_tell_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_window.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_write.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_zip.c',
        '../src/MrEngine/Dep/libzip/source/zip_source_zip_new.c',
        '../src/MrEngine/Dep/libzip/source/zip_stat.c',
        '../src/MrEngine/Dep/libzip/source/zip_stat_index.c',
        '../src/MrEngine/Dep/libzip/source/zip_stat_init.c',
        '../src/MrEngine/Dep/libzip/source/zip_strerror.c',
        '../src/MrEngine/Dep/libzip/source/zip_string.c',
        '../src/MrEngine/Dep/libzip/source/zip_unchange.c',
        '../src/MrEngine/Dep/libzip/source/zip_unchange_all.c',
        '../src/MrEngine/Dep/libzip/source/zip_unchange_archive.c',
        '../src/MrEngine/Dep/libzip/source/zip_unchange_data.c',
        '../src/MrEngine/Dep/libzip/source/zip_utf-8.c',
    }

    if IS_VS then
        files{
            '../src/MrEngine/Dep/libzip/source/zip_crypto_win.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_file_win32.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_file_win32_ansi.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_file_win32_named.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_file_win32_utf16.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_file_win32_utf8.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_winzip_aes_decode.c',
            '../src/MrEngine/Dep/libzip/source/zip_source_winzip_aes_encode.c',
            '../src/MrEngine/Dep/libzip/source/zip_winzip_aes.c',
        }
    else
        files{
            '../src/MrEngine/Dep/libzip/source/zip_source_file_stdio_named.c',
            '../src/MrEngine/Dep/libzip/source/zip_random_unix.c',
			'../src/MrEngine/Dep/libzip/source/zip_mkstempm.c',
        }
    end

    includedirs {
        '../src/MrEngine/Dep/libzip/include',
        '../src/MrEngine/Dep/zlib',
    }

    if IS_XCODE then 
        configuration { "Release" }
            flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "Symbols" } 
    end
    targetname "libzip"

project "zipper"
	kind "StaticLib"
	language "C++"

	files{
		'../src/MrEngine/Dep/zipper/zipper/CDirEntry.h',
		'../src/MrEngine/Dep/zipper/zipper/CDirEntry.cpp',
		'../src/MrEngine/Dep/zipper/zipper/defs.h',
		'../src/MrEngine/Dep/zipper/zipper/tools.h',
		'../src/MrEngine/Dep/zipper/zipper/tools.cpp',
		'../src/MrEngine/Dep/zipper/zipper/unzipper.h',
		'../src/MrEngine/Dep/zipper/zipper/unzipper.cpp',
		'../src/MrEngine/Dep/zipper/zipper/zipper.h',
		'../src/MrEngine/Dep/zipper/zipper/zipper.cpp',
		'../src/MrEngine/Dep/zipper/zipper/tps/dirent.h',
		'../src/MrEngine/Dep/zipper/zipper/tps/dirent.c',
		'../src/MrEngine/Dep/zipper/minizip/crypt.h',
		'../src/MrEngine/Dep/zipper/minizip/ioapi.h',
		'../src/MrEngine/Dep/zipper/minizip/ioapi.c',
		'../src/MrEngine/Dep/zipper/minizip/ioapi_buf.h',
		'../src/MrEngine/Dep/zipper/minizip/ioapi_buf.c',
		'../src/MrEngine/Dep/zipper/minizip/ioapi_mem.h',
		'../src/MrEngine/Dep/zipper/minizip/ioapi_mem.c',
		'../src/MrEngine/Dep/zipper/minizip/iowin32.h',
		'../src/MrEngine/Dep/zipper/minizip/iowin32.c',
		'../src/MrEngine/Dep/zipper/minizip/unzip.h',
		'../src/MrEngine/Dep/zipper/minizip/unzip.c',
		'../src/MrEngine/Dep/zipper/minizip/zip.h',
		'../src/MrEngine/Dep/zipper/minizip/zip.c',
	}

	includedirs {
		'../src/MrEngine/Dep/zipper/minizip',
		'../src/MrEngine/Dep/zlib',
	}

	if IS_XCODE then 
		configuration { "Release" }
			flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "Symbols" } 
	end
	targetname "zipper"

project 'CompilerShader'
	kind 'ConsoleApp'
	language 'C++'
	
	flags { "Cpp11" }
	defines { 'OF_LUA_CPP' }

	includedirs {
		'../src/MrEngine/Dep',
		'../src/MrEngine/Dep/liblz4/include',
		'../src/MrEngine/Dep/zlib',
	}

	files {
		'../src/MrEngine/Tools/CompilerShader/**.*',
	}

	links { 'mcpp', 'hlslcc_lib', 'lua', 'zlib', 'zipper', 'libzip', 'liblz4' }

    -- links { "MrEngine" }
	-- if IS_VS then
	-- 	links { 'opengl32','d3d11','d3dcompiler','winmm' }
	-- end

	targetname 'CompilerShader'

