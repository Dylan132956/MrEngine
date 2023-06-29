
#pragma once

#include <string>
#include <vector>
#include <map>
#include "mcpp-2.7.2/inc/mcpp.h"
#include "../complierState.h"
#include "../utils.h"

class ShaderProcesser
{
public:
	ShaderProcesser(ComplierState& complierState)
		:pState(&complierState)
	{
		std::string shaderSource = Format("%s\n", complierState.active.curOut.unCompileShader.c_str());
		shaderFileContents["curShader"] = shaderSource;
	}

	file_loader GetLoaderInterface();

	static int GetFileContents(void* InUserData, const char* InShaderFilePath, const char** OutContents, size_t* OutContentSize);

	bool Run();

private:
	ComplierState* pState;
	std::map<std::string, std::string> shaderFileContents;
};