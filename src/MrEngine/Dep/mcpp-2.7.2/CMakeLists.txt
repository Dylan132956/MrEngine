include(${ROOT_PATH}/build/cmake/CPlatform.txt)

set(PROJECT_NAME mcpp)

################################################################################
# Source groups
################################################################################

set(
    ALL_FILES
    "src/configed.H"
    "src/directive.c"
    "src/eval.c"
    "src/expand.c"
    "src/internal.H"
    "src/main.c"
    "src/mbchar.c"
    "inc/mcpp.h"
    "src/mcpp_lib.h"
    "src/mcpp_out.h"
    "src/noconfig.H"
    "src/preproc.c"
    "src/support.c"
    "src/system.c"
    "src/system.H"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ROOT_PATH}/binary/function/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_EDITOR_DEBUG ${ROOT_PATH}/binary/function/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ROOT_PATH}/binary/function/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_EDITOR_RELEASE ${ROOT_PATH}/binary/function/)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "third")

# use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
#set(ROOT_NAMESPACE libzip)

################################################################################
# Target name
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Any CPU")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG          "${PROJECT_NAME}_d"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG          "${PROJECT_NAME}_d"
        TARGET_NAME_EDITOR_DEBUG   "${PROJECT_NAME}_d"
    )
endif()

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Any CPU")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_EDITOR_DEBUG   "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_EDITOR_RELEASE "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_RELEASE        "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_EDITOR_RELEASE "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_RELEASE        "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_EDITOR_RELEASE "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_RELEASE        "TRUE"
    )
endif()
################################################################################
# Include directories
################################################################################
if(NOT MSVC)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

################################################################################
# Compile definitions
################################################################################
if(NOT MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "HAVE_UNISTD_H;"
        "HAVE_STRINGS_H;"
        "HAVE_STRCASECMP;" 
        "HAVE_FSEEKO;" 
        "HAVE_FTELLO;" 
        "HAVE_STRICMP;"
        "NDEBUG;"
        "_MBCS"
        "MCPP_LIB=1"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "ZIP_STATIC;"
        "WIN32;"
        "_WINDOWS;"
        "NDEBUG;"
        "_CRT_SECURE_NO_WARNINGS;"
        "_CRT_NONSTDC_NO_DEPRECATE;"
        "_MBCS"
        "MCPP_LIB=1"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "ZIP_STATIC;"
        "WIN32;"
        "_WINDOWS;"
        "NDEBUG;"
        "_CRT_SECURE_NO_WARNINGS;"
        "_CRT_NONSTDC_NO_DEPRECATE"
        "_MBCS"
        "MCPP_LIB=1"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Any CPU")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Editor_Debug>:
                /O2;
                /Oi;
                /Gy
            >
            $<$<CONFIG:Editor_Release>:
                /O2;
                /Oi;
                /Gy
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy
            >
            /permissive-;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Editor_Debug>:
                /Od
            >
            $<$<CONFIG:Editor_Release>:
                /O2;
                /Oi;
                /Gy
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy
            >
            /permissive-;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Editor_Debug>:
                /Od
            >
            $<$<CONFIG:Editor_Release>:
                /O2;
                /Oi;
                /Gy
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy
            >
            /permissive-;
            /W3;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Any CPU")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Editor_Debug>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:Editor_Release>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Editor_Release>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Editor_Release>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
        )
    endif()
endif()

################################################################################
# Dependencies
################################################################################












