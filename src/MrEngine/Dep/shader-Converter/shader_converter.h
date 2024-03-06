#pragma once

#include <string>
#include <vector>
#include "glslang/Public/ShaderLang.h"
#include "SPIRV-Cross/spirv_glsl.hpp"

struct compile_arguments
{
	bool set_es = false;
	bool es = false;
	size_t version = 0;
	bool set_version = false;
};

void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);
std::string spirv_converter(compile_arguments& arg, std::vector<uint32_t>& spirv_file);

