#pragma once 

#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include "complierParam.h"

enum ShaderAPI
{
	OPENGL,
	GLES2,
	GLES31,
	Metal,
	D3D11,
	D3D12,
	VULKAN,
};

enum ShaderStage
{
	VERTEX_SHADER_Stage,
	PIXEL_SHADER_Stage,
	COMPUTE_SHADER_Stage,
};

struct Uniform
{
	std::string name;
	std::string type;
	std::string sit;
	std::string num;
	std::string regIndex;
	std::string regCount;
	std::vector<unsigned> subIndexs;
};

bool operator == (const Uniform & op1, const Uniform & op2);

namespace std
{
	template<>
	struct hash<Uniform>
	{
		std::size_t operator()(Uniform const & v) const noexcept
		{
			return hash<std::string>{}(v.name) ^
				hash<std::string>{}(v.type) ^
				hash<std::string>{}(v.sit) ^
				hash<std::string>{}(v.num) ^
				hash<std::string>{}(v.regIndex) ^
				hash<std::string>{}(v.regCount);
		}
	};
}

struct Attribute
{
	std::string attName;
	std::string attID;
};

bool operator == (const Attribute & op1, const Attribute & op2);

namespace std
{
	template<>
	struct hash<Attribute>
	{
		std::size_t operator()(Attribute const & v) const noexcept
		{
			return hash<std::string>{}(v.attName) ^ hash<std::string>{}(v.attID);
		}
	};
}
class ShaderOut
{
public:
	std::vector<std::string> keyWords;
	std::vector<std::string> systemKeyWords;
	std::vector<Uniform> vsUniforms;
	std::vector<Uniform> psUniforms;
	std::vector<Uniform> csUniforms;
	std::string unCompileShader;
	std::string compileVShader;
	std::string compilePShader;
	std::string compileCShader;
	std::string error;
	std::string preCompileShader;
	std::string vsConstantSize;
	std::string psConstantSize;
	std::string csConstantSize;
	std::string threadNumX;
	std::string threadNumY;
	std::string threadNumZ;
	ShaderAPI shaderAPI;
	std::vector<Attribute> vsAttributes;

	void Reset()
	{
		keyWords.clear();
		compileVShader = "";
		compilePShader = "";
		compileCShader = "";
		error = "";
		preCompileShader = "";
		vsConstantSize = "0";
		psConstantSize = "0";
		csConstantSize = "0";
		vsUniforms.clear();
		psUniforms.clear();
		csUniforms.clear();
		vsAttributes.clear();
		systemKeyWords.clear();
	}
};

class ShaderCollection
{
public:
	size_t beginPos;
	size_t endPos;
	std::string shaderSeg;
	ShaderOut curOut;
	ComplierParam param;
	std::vector<ShaderOut> shaderOuts;
};

enum class CompressMode
{
	E_FORCE_COMPRESS,
	E_FORCE_NOT_COMPRESS,
	E_DEFINE_FILE_SIZE,
};

class ComplierState
{
public:
	~ComplierState() {}
	std::string curPath;
	std::vector<std::string> includePaths;
	std::vector<ShaderAPI> curShaderAPIs;
	std::string shaderSource;
	std::string outFinalShader;
	std::string stripShader;
	ShaderCollection active;
	std::vector<ShaderCollection> shaderCollect;
	CompressMode compressShader = CompressMode::E_DEFINE_FILE_SIZE;
	bool isCompress = false;
	std::vector<std::string> compressShaders;
};

bool ProcessShader(ComplierState& gloablState);
bool OutputCompliedShader(ComplierState& globalState);
bool CheckError(ComplierState& globalState);
