#pragma once
#include "../Shader.h"

#include <string>
#include <vector>
namespace moonriver {
	void converter_spirv(int stage, const char* text[], const std::string fileName[],
		const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv, Shader::Pass* pass = nullptr);
}
