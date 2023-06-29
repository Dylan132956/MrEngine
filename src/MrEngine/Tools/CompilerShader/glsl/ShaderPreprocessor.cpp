

#include "hlslcc_lib/ShaderCompilerCommon.h"
#include "ShaderPreprocessor.h"
#include "hlslcc_lib/HlslccDefinitions.h"

#include <unordered_map>

file_loader ShaderProcesser::GetLoaderInterface()
{
	file_loader Loader;
	Loader.get_file_contents = GetFileContents;
	Loader.user_data = (void*)this;
	return Loader;
}

int ShaderProcesser::GetFileContents(void* InUserData, const char* InShaderFilePath, const char** OutContents, size_t* OutContentSize)
{
	ShaderProcesser* processer = (ShaderProcesser*)InUserData;

	std::string filePathStr = InShaderFilePath;

	std::string::size_type fsize = filePathStr.find_last_of("/");
	if (fsize != std::string::npos)
	{
		filePathStr = filePathStr.substr(fsize + 1);
	}

	std::string* fileContents = nullptr;
	auto it = processer->shaderFileContents.find(filePathStr);
	if (it != processer->shaderFileContents.end())
	{
		fileContents = &(it->second);
	}
	else
	{
		std::string contents;
		for (const auto& path : processer->pState->includePaths)
		{
			if (ReadFileContent(path + "\\" + filePathStr, contents))
			{
				std::string appendSource = Format("%s\n", contents.c_str());
				processer->shaderFileContents.insert({ filePathStr, appendSource });
				fileContents = &(processer->shaderFileContents[filePathStr]);
				break;
			}
		}
	}

	if (OutContents)
	{
		*OutContents = (fileContents != nullptr ? fileContents->c_str() : nullptr);
	}

	if (OutContentSize)
	{
		*OutContentSize = (fileContents != nullptr ? fileContents->size() : 0);
	}

	return fileContents != nullptr;
}

bool ShaderProcesser::Run()
{
	std::vector<std::string> options{ "-V199901L"};
	std::vector<const char* > optionsChars;
	std::vector<std::string>& keyWords = pState->active.curOut.keyWords;
	std::vector<std::string>& systemKeyWords = pState->active.curOut.systemKeyWords;

	for (auto& key : keyWords)
	{
		std::string strValue = Format("-D%s=%s", key.c_str(), "1");
		options.push_back(strValue);
	}

	for (auto& key : systemKeyWords)
	{
		std::string strValue = Format("-D%s=%s", key.c_str(), "1");
		options.push_back(strValue);
	}

	for (auto& key : options)
	{
		optionsChars.push_back(key.c_str());
	}

	char* outShader = nullptr;
	char* outError = nullptr;

	int result = 0;

	result = mcpp_run(
		optionsChars.data(),
		optionsChars.size(),
		"curShader",
		&outShader,
		&outError,
		this->GetLoaderInterface()
	);

	if (outError)
	{
		std::string mcpError = Format("--------------ShaderProcesser Error------------------------\n%s", outError);
		pState->active.curOut.error = mcpError;
		return false;
	}
	else
	{
		pState->active.curOut.unCompileShader = outShader;
	}

	return true;
}
