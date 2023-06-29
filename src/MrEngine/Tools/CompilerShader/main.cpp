
#include "utils.h"
#include "complierState.h"
#include "shaderpack.h"
#include "lua/lua.hpp"

#include <unordered_set>
#include <unordered_map>

using namespace std;
using namespace OrangeFilter;

struct CmdArgs
{
	std::string includePath;
	std::string outputPath;
	std::string inputPath;
	std::vector<ShaderAPI> APIs;
	std::unordered_set<std::string> shaderTypes;
	CompressMode compressShader = CompressMode::E_DEFINE_FILE_SIZE;
};

static bool parse_cmd_args(int argc, char* const argv[], CmdArgs& cmdArgs)
{
	int index = 1;

	while (index < argc)
	{
		std::string s = argv[index];
		if (s == "-i")
		{
			if (index + 1 >= argc)
			{
				return false;
			}
			cmdArgs.inputPath = argv[index + 1];
			index += 2;
		}
		else if (s == "-o")
		{
			if (index + 1 >= argc)
			{
				return false;
			}
			cmdArgs.outputPath = argv[index + 1];
			index += 2;
		}
		else if (s == "-inc")
		{
			if (index + 1 >= argc)
			{
				return false;
			}
			cmdArgs.includePath = argv[index + 1];
			index += 2;
		}
		else if (s == "-api")
		{
			if (index + 1 >= argc)
			{
				return false;
			}
			std::unordered_map<std::string, ShaderAPI> map({
				{ "OPENGL", ShaderAPI::OPENGL },
				{ "GLES2", ShaderAPI::GLES2 },
				{ "GLES31", ShaderAPI::GLES31 },
				{ "METAL", ShaderAPI::Metal },
				{ "D3D11", ShaderAPI::D3D11 },
				{ "D3D12", ShaderAPI::D3D12 },
				{ "VULKAN", ShaderAPI::VULKAN },
				});
			auto apis = split(argv[index + 1], ",");
			for (auto& api : apis)
			{
				if (map.find(api) == map.end())
				{
					return false;
				}
				cmdArgs.APIs.push_back(map[api]);
			}
			index += 2;
		}
		else if (s == "-c")
		{
			if (index + 1 >= argc)
			{
				return false;
			}
			std::string strcomress = argv[index + 1];
			if (strcomress == "YES")
			{
				cmdArgs.compressShader = CompressMode::E_FORCE_COMPRESS;
			}
			else if (strcomress == "NO")
			{
				cmdArgs.compressShader = CompressMode::E_FORCE_NOT_COMPRESS;
			}
			else if (strcomress == "FILESIZE")
			{
				cmdArgs.compressShader = CompressMode::E_DEFINE_FILE_SIZE;
			}
			index += 2;
		}
		else
		{
			return false;
		}
	}
	if (cmdArgs.inputPath == "" ||
		cmdArgs.includePath == "" ||
		cmdArgs.outputPath == "")
	{
		return false;
	}
	if (cmdArgs.APIs.size() == 0)
	{
		cmdArgs.APIs = std::vector<ShaderAPI>({
			ShaderAPI::OPENGL,
			ShaderAPI::GLES2,
			ShaderAPI::GLES31,
			ShaderAPI::Metal,
			ShaderAPI::D3D11,
			ShaderAPI::D3D12,
			ShaderAPI::VULKAN,
			});
	}
	return true;
}

extern const char* luaToJsonCode;
extern const char* converterCode;
extern const char* converterCodeComputer;
static bool ConvertToJson(const std::string& luaPath, std::string& jsonPath, bool bComputer)
{
	int err = LUA_OK;
	jsonPath = luaPath;
	ReplaceString(jsonPath, ".lua", ".json");
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	try
	{
		err = luaL_loadstring(L, bComputer ? converterCodeComputer : converterCode);
		if (err != LUA_OK)
		{
			throw Format("error in luaL_loadfile\n");
		}
		err = lua_pcall(L, 0, 1, 0);
		if (err != LUA_OK)
		{
			throw Format("error in lua_pcall: %s\n", lua_tostring(L, -1));
		}
		err = luaL_loadstring(L, luaToJsonCode);
		if (err != LUA_OK)
		{
			throw Format("error in luaL_loadfile 2\n");
		}
		err = lua_pcall(L, 0, 1, 0);
		if (err != LUA_OK)
		{
			throw Format("error in lua_pcall 2: %s\n", lua_tostring(L, -1));
		}
		lua_pushstring(L, luaPath.c_str());
		lua_pushstring(L, jsonPath.c_str());
		err = lua_pcall(L, 3, 0, 0);
		if (err != LUA_OK)
		{
			throw Format("error in lua_pcall 3: %s\n", lua_tostring(L, -1));
		}
	}
	catch (std::string msg)
	{
		printf("%s", msg.c_str());
		lua_close(L);
		return false;
	}
	lua_close(L);
	return true;
}

int main(int argc, char* const argv[])
{
	CmdArgs cmdArgs;
	if (!parse_cmd_args(argc, argv, cmdArgs))
	{
		printf("arg error!\n");
		return 1;
	}
	 
	std::string & inputPath = cmdArgs.inputPath;
	std::string & commonIncludePath = cmdArgs.includePath;
	std::string & outputPath = cmdArgs.outputPath;

#if defined(_WIN32) || defined(_WIN64)
	string_replace(inputPath, "/", "\\");
	string_replace(commonIncludePath, "/", "\\");
	string_replace(outputPath, "/", "\\");
#endif

	std::string curPath = GetPathOrURLShortName(inputPath);

	std::vector<std::string> includesPath;
	includesPath.push_back(".");
	if (curPath != "." PATH_SEP)
	{
		includesPath.push_back(curPath);
	}
	if (curPath != commonIncludePath)
	{
		includesPath.push_back(commonIncludePath);
	}

	FILE* file = NULL;
	fopen_s(&file, inputPath.c_str(), "rt");
	if (file == NULL)
		return 2;

	fseek(file, 0, SEEK_END);
	int rawLength = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (rawLength < 0)
	{
		fclose(file);
		PrintError("read file failed");
		return 3;
	}

	std::string readTexts;
	readTexts.resize(rawLength);
	int readLength = fread(&*readTexts.begin(), 1, rawLength, file);
	fclose(file);
	readTexts.resize(readLength);
	RemoveUTF8BOM(readTexts);

	//  ------------------- compile -----------------
	ComplierState globalState;

	if (cmdArgs.compressShader != CompressMode::E_DEFINE_FILE_SIZE)
	{
		globalState.compressShader = cmdArgs.compressShader;
	}
	
	globalState.curPath = argv[0];
	globalState.includePaths = includesPath;
	globalState.shaderSource = readTexts;
	globalState.curShaderAPIs = cmdArgs.APIs;
	ProcessShader(globalState);
	if (!globalState.active.param.csName.empty() && globalState.curShaderAPIs[0] == GLES2)
	{
		return 4;
	}

	OutputCompliedShader(globalState);
	if (CheckError(globalState))
	{
		return 5;
	}

	WriteText(outputPath, globalState.outFinalShader);

	// lua to json
	std::string luaShaderPath = outputPath;
	{
		std::string jsonShaderPath;
		bool bComputer = !globalState.active.param.csName.empty();
		bool ret = ConvertToJson(luaShaderPath, jsonShaderPath, bComputer);
		if (ret == true)	
		{
			remove(luaShaderPath.c_str()); 
		}
	}

	if (globalState.isCompress)
	{
		std::string shaderName = luaShaderPath;
		string_replace(shaderName, ".lua", "");
		std::string comShaderName = shaderName + ".code";
		ShaderPack(comShaderName, globalState);
	}

    return 0;
}


