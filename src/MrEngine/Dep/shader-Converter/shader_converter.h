#pragma once

#include <string>
#include <vector>
#include "glslang/Public/ShaderLang.h"
#include "SPIRV-Cross/spirv_glsl.hpp"

struct compile_arguments
{
    //gl gles
	bool set_es = false;
	bool es = false;
    unsigned int version = 0;
	bool set_version = false;
    //msl
    bool msl = false;
    bool msl_ios = false;
    bool set_msl_version = false;
    unsigned int msl_version = 0;
    bool msl_decoration_binding = false;
};

void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);
std::string spirv_converter(compile_arguments& arg, std::vector<uint32_t>& spirv_file);

