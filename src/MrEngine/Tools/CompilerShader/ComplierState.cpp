
#include <limits.h>
#include <stdio.h>
#include <clocale>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdarg.h>
#include "complierParam.h"
#include "macroEnumer.h"
#include "utils.h"
//#include "hlslCompiler.h"
#include "complierState.h"
#include "glsl/GLSLCompiler.h"
#include "hlsl/hlslCompiler.h"

#include <set>
#include <assert.h>

using namespace std;

static std::string constbuffername = "constBuffer";

void GetValidApis(ComplierState& gloablState, std::vector<ShaderAPI>& outAPIS)
{
	const std::vector<ShaderAPI>& shaderApis = gloablState.curShaderAPIs;
	const ComplierParam& param = gloablState.active.param;
	bool hasComputerShader = (!param.csName.empty());

	for (int i = 0; i < shaderApis.size(); i++)
	{
		ShaderAPI curApi = shaderApis[i];

		if (hasComputerShader && curApi == ShaderAPI::GLES2)
		{
			continue;
		}

		outAPIS.push_back(curApi);
	}
}

bool ProcessShaderSeg(ComplierState& gloablState)
{
	string& shaderSeg = gloablState.active.shaderSeg;
	Macros macro = Macros::CreateFormShaderSource(shaderSeg);
	ComplierParam param = ComplierParam::CreateFromString(shaderSeg);
	gloablState.active.param = param;
	gloablState.active.shaderOuts.clear();

	if (param.HasProgram())
	{
		std::vector<ShaderAPI> shaderApis;
		GetValidApis(gloablState, shaderApis);

		for (int i = 0; i < shaderApis.size(); i++)
		{
			ShaderAPI curApi = shaderApis[i];

			MacroEnumer enumer(macro);
			enumer.Begin();

			while (enumer.HasElement())
			{
				ShaderOut& shaderOut = gloablState.active.curOut;
				shaderOut.Reset();

				enumer.GetCurKeyWords(shaderOut.keyWords);
				shaderOut.shaderAPI = curApi;
				// complie shader
				if (curApi == ShaderAPI::D3D11
					|| curApi == ShaderAPI::D3D12)
				{
					ComplieHLSLShader(gloablState);
				}
				else if (curApi == ShaderAPI::VULKAN)
				{
					//ComplieSpirvShader(gloablState);
				}
				else if (curApi == ShaderAPI::GLES2
					|| curApi == ShaderAPI::GLES31
					|| curApi == ShaderAPI::OPENGL
					|| curApi == ShaderAPI::Metal)
				{
					CrossComplie(curApi, gloablState);
				}

				gloablState.active.shaderOuts.push_back(shaderOut);

				// error check
				if (!shaderOut.error.empty())
				{
					break;
				}
				enumer.Next();
			}
		}
	}

	return true;
}

bool ProcessShader(ComplierState& gloablState)
{
	static std::string cgprogram("CGPROGRAM");
	static std::string endcg("ENDCG");

	size_t beginToken = std::string::npos;
	size_t endToken = std::string::npos;

	bool bfind = FindInTokensString(gloablState.shaderSource, cgprogram,
		endcg, 0, beginToken, endToken);

	while (bfind)
	{
		ShaderCollection& shaderCol = gloablState.active;
		shaderCol.beginPos = beginToken;
		shaderCol.endPos = endToken + endcg.length();
		shaderCol.shaderSeg = gloablState.shaderSource.substr(beginToken + cgprogram.length(),
			endToken - beginToken - cgprogram.length());
		std::string outShaderSeg = "";

		ProcessShaderSeg(gloablState);

		gloablState.shaderCollect.push_back(shaderCol);
		size_t preEndToken = endToken;
		bfind = FindInTokensString(gloablState.shaderSource, cgprogram,
			endcg, endToken, beginToken, endToken);

		if (!bfind)
		{
			break;
		}
	}

	return true;
}

string shaderAPI2Str(ShaderAPI api)
{
	string str;
	if (api == ShaderAPI::OPENGL)
	{
		str = "opengl";
	}
	else if (api == ShaderAPI::GLES2)
	{
		str = "gles2";
	}
	else if (api == ShaderAPI::GLES31)
	{
		str = "gles31";
	}
	else if (api == ShaderAPI::Metal)
	{
		str = "metal";
	}
	else if (api == ShaderAPI::D3D11)
	{
		str = "d3d11";
	}
	else if (api == ShaderAPI::D3D12)
	{
		str = "d3d12";
	}
	else if (api == ShaderAPI::VULKAN)
	{
		str = "vulkan";
	}
	return str;
}

string ParsePropertyContent(const string& inProperty)
{
	const string beginparam = "[";
	const string endparam = "]";
	size_t beginToken = std::string::npos;
	size_t endToken = std::string::npos;
	std::string property;

	property += Format("\tProperties = \n");
	property += Format("\t{\n");
	property += Format("\t\t%s\n", inProperty.c_str());
	property += Format("\t}\n");

	//
	size_t startPos = 0;
	bool bfind = FindInTokensString(property, beginparam,
		endparam, startPos, beginToken, endToken);

	while (bfind)
	{
		size_t attributeStart = beginToken;
		size_t attributeCount = endToken - beginToken + 1;
		property.erase(attributeStart, attributeCount);

		startPos = attributeStart;
		bfind = FindInTokensString(property, beginparam,
			endparam, startPos, beginToken, endToken);
	}

	return property;
}

string ParsePropertyAttribute(const string& propertyparam)
{
	const string beginparam = "[";
	const string endparam = "]";
	size_t beginToken = std::string::npos;
	size_t endToken = std::string::npos;
	std::string propertyAttribute;

	propertyAttribute += Format("\tAttributes = \n");
	propertyAttribute += Format("\t{\n");

	size_t startPos = 0;
	bool bfind = FindInTokensString(propertyparam, beginparam,
		endparam, startPos, beginToken, endToken);

	while (bfind)
	{
		size_t attributeStart = beginToken + beginparam.length();
		size_t attributeCount = endToken - (beginToken + beginparam.length());
		string paramAttribute = propertyparam.substr(attributeStart, attributeCount);
		string paramName = FindNextWord(propertyparam, endToken + endparam.length());
		StringTrim(paramName);
		StringTrim(paramAttribute);

		propertyAttribute += Format("\t\t%s = \"%s\", ", paramName.c_str(), paramAttribute.c_str());
		propertyAttribute += "\n";
		startPos = endToken + endparam.length();
		bfind = FindInTokensString(propertyparam, beginparam,
			endparam, startPos, beginToken, endToken);
	}

	propertyAttribute += Format("\t}\n");
	return propertyAttribute;
}

bool operator == (const Uniform& op1, const Uniform& op2)
{
	return op1.name == op2.name &&
		op1.type == op2.type &&
		op1.sit == op2.sit &&
		op1.num == op2.num &&
		op1.regIndex == op2.regIndex &&
		op1.regCount == op2.regCount;
}

bool operator == (const Attribute& op1, const Attribute& op2)
{
	return op1.attName == op2.attName && op1.attID == op2.attID;
}

bool OutputCompliedShader(ComplierState& globalState)
{
	vector<ShaderCollection>& shaderCollect = globalState.shaderCollect;
	size_t size = shaderCollect.size();
	globalState.outFinalShader = globalState.shaderSource;
	string& finalShader = globalState.outFinalShader;
	bool isCompute = false;
	int compressIndex = 0;
	std::vector<std::vector<unsigned char>> compressShader;

	if (globalState.compressShader == CompressMode::E_FORCE_COMPRESS)
	{
		globalState.isCompress = true;
	}
	else if (globalState.compressShader == CompressMode::E_FORCE_NOT_COMPRESS)
	{
		globalState.isCompress = false;
	}
	else
	{
		int totalOutSize = 0;
		for (int i = size - 1; i >= 0; i--)
		{
			ShaderCollection& curCollect = shaderCollect[i];
			std::vector<ShaderOut>& shaderOut = curCollect.shaderOuts;

			for (int j = 0; j < shaderOut.size(); j++)
			{
				ShaderOut& curOut = shaderOut[j];

				totalOutSize += curOut.compileVShader.size();
				totalOutSize += curOut.compilePShader.size();
				totalOutSize += curOut.compileCShader.size();
			}
		}
		globalState.isCompress = totalOutSize >= 10 * 1024;
	}

	for (int i = size - 1; i >= 0; i--)
	{
		ShaderCollection& curCollect = shaderCollect[i];
		size_t beginPos = curCollect.beginPos;
		size_t endPos = curCollect.endPos;

		string insertString;

		std::vector<ShaderOut>& shaderOut = curCollect.shaderOuts;

		std::unordered_map<unsigned, Uniform> uniformHashCodeMap;
		std::unordered_map<Uniform, size_t> uniformMap;
		std::vector<Uniform> uniformList;

		std::unordered_map<Attribute, size_t> attributeMap;
		std::vector<Attribute> attributeList;

#define DATA_COPY(dmap, dlist, src_data) \
	for (auto & u : src_data)\
	{\
		if (dmap.find(u) == dmap.end())\
		{\
			uniformHashCodeMap.insert({std::hash<Uniform>{}(u), u});\
			dmap.insert({ u, dmap.size() });\
			dlist.push_back(u);\
		}\
	}

		for (size_t j = 0; j < shaderOut.size(); j++)
		{
			ShaderOut& curOut = shaderOut[j];

			DATA_COPY(uniformMap, uniformList, curOut.vsUniforms);
			DATA_COPY(uniformMap, uniformList, curOut.psUniforms);
			DATA_COPY(uniformMap, uniformList, curOut.csUniforms);

			for (auto& u : curOut.vsAttributes)
			{
				if (attributeMap.find(u) == attributeMap.end())
				{
					attributeMap.insert({ u, attributeMap.size() });
					attributeList.push_back(u);
				}
			}
		}

		for (auto& u : uniformList)
		{
			std::vector<unsigned> data(u.subIndexs.size());
			for (auto& uhash : u.subIndexs)
			{
				uhash = uniformMap[uniformHashCodeMap[uhash]];
			}
		}

#undef DATA_COPY


		// 导出处理const buffer里包含的类型
		auto processConstBufferUniform = [&](std::vector<Uniform>& unis) -> void
		{
			std::set<int> eraseUniIndexs;	// 记录要删除的索引
			for (auto& u : unis)
			{
				for (auto idx : u.subIndexs)
				{
					eraseUniIndexs.insert(uniformMap[uniformHashCodeMap[idx]]);
				}
			}

			for (auto idx : eraseUniIndexs)
			{
				auto iter = unis.begin();
				while (iter != unis.end())
				{
					if (std::hash<Uniform>{}(*iter) == std::hash<Uniform>{}(uniformList[idx]))
					{
						iter = unis.erase(iter);
					}
					else
					{
						++iter;
					}
				}
			}
		};

		auto processUniforms = [&](ShaderOut& curOut) -> void
		{
			processConstBufferUniform(curOut.vsUniforms);
			processConstBufferUniform(curOut.psUniforms);

			if (curOut.shaderAPI == OPENGL || curOut.shaderAPI == GLES2 || curOut.shaderAPI == GLES31)
			{
				std::set<unsigned> hashcache;
				for (auto& u : curOut.vsUniforms)
				{
					hashcache.insert(std::hash<Uniform>{}(u));
				}
				for (auto& u : curOut.psUniforms)
				{
					auto ush = std::hash<Uniform>{}(u);
					if (hashcache.find(ush) == hashcache.end())
					{
						curOut.vsUniforms.insert(curOut.vsUniforms.end(), u);
					}
				}
				curOut.psUniforms.clear();
			}
		};

		insertString += Format("\nUNIFORMS =\n");
		insertString += Format("{\n");
		for (auto& u : uniformList)
		{
			insertString += Format("			{\n");
			insertString += Format("				varName = \"%s\",\n", u.name.c_str());
			insertString += Format("				varType = \"%s\",\n", u.type.c_str());
			insertString += Format("				varSit = \"%s\",\n", u.sit.c_str());
			insertString += Format("				varNum = \"%s\",\n", u.num.c_str());
			insertString += Format("				varRegIndex = \"%s\",\n", u.regIndex.c_str());
			insertString += Format("				varRegCount = \"%s\",\n", u.regCount.c_str());
			if (!u.subIndexs.empty())
			{
				insertString += Format("				subVar = {", u.regCount.c_str());
				for (auto ui : u.subIndexs)
				{
					insertString += Format("%u, ", ui);
				}
				insertString += Format("}\n", u.regCount.c_str());
			}
			insertString += Format("			},\n");
		}
		insertString += Format("}\n");
		//uniformList.clear();

		insertString += Format("\nATTRIBUTES =\n");
		insertString += Format("{\n");
		for (auto& a : attributeList)
		{
			insertString += Format("			{\n");
			insertString += Format("				attName = \"%s\",\n", a.attName.c_str());
			insertString += Format("				attID = \"%s\",\n", a.attID.c_str());
			insertString += Format("			},\n");
		}
		insertString += Format("}\n");
		attributeList.clear();

		insertString += Format("\nPrograms =\n");
		insertString += Format("{\n");

		for (size_t j = 0; j < shaderOut.size(); j++)
		{
			ShaderOut& curOut = shaderOut[j];

			std::string fixVertex;
			std::string fixPixel;
			std::string fixCompute;

			if (globalState.isCompress)
			{
				// shader如果压缩的话, 会把shader的相关数据放到另外一个文件中来做压缩处理
				// 这里会把vs和ps合并，这样压缩后会小一点
				std::string inputStr = curOut.compileVShader + curOut.compilePShader + curOut.compileCShader;
				globalState.compressShaders.emplace_back(std::move(inputStr));

				int blockIndex = globalState.compressShaders.size() - 1;
				int off = 0;
				int sz = curOut.compileVShader.size();
				fixVertex = Format("COMP,%d,%d,%d", blockIndex, off, sz);
				off += sz;
				sz = curOut.compilePShader.size();
				fixPixel = Format("COMP,%d,%d,%d", blockIndex, off, sz);
				off += sz;
				sz = curOut.compileCShader.size();
				if (sz > 0)
				{
					fixCompute = Format("COMP,%d,%d,%d", blockIndex, off, sz);
				}
			}
			else
			{
				fixVertex = curOut.compileVShader;
				fixPixel = curOut.compilePShader;
				fixCompute = curOut.compileCShader;
			}

			// 不在使用lua的转义
			//ReplaceString(fixVertex, "[[", "\\[\\[");
			//ReplaceString(fixVertex, "]]", "\\]\\]");
			//ReplaceString(fixPixel, "[[", "\\[\\[");
			//ReplaceString(fixPixel, "]]", "\\]\\]");

			insertString += Format("	{\n");
			insertString += Format("		shaderApi = \"%s\",\n", shaderAPI2Str(curOut.shaderAPI).c_str());
			insertString += Format("		keyWords = {%s},\n", Vector2String(curOut.keyWords).c_str());

			processUniforms(curOut);

			{
				insertString += Format("		vsBufferSize = \"%s\",\n", curOut.vsConstantSize.c_str());

				// output vs attributes
				insertString += Format("		vsAttributes = {");
				for (auto& a : curOut.vsAttributes)
				{
					insertString += Format("%u,", attributeMap[a]);
				}
				insertString += Format("},\n");

				// output vs uniform
				insertString += Format("		vsUniforms = {");
				for (auto& u : curOut.vsUniforms)
				{
					insertString += Format("%u,", uniformMap[u]);
				}
				insertString += Format("},\n");
			}

			{
				insertString += Format("		psBufferSize = \"%s\",\n", curOut.psConstantSize.c_str());

				insertString += Format("		psUniforms = {");
				for (auto& u : curOut.psUniforms)
				{
					insertString += Format("%u,", uniformMap[u]);
				}
				insertString += Format("},\n");
			}

			if (!fixCompute.empty())
			{
				isCompute = true;
				insertString += Format("		csBufferSize = \"%s\",\n", curOut.csConstantSize.c_str());

				insertString += Format("		csUniforms = {");
				for (auto& u : curOut.csUniforms)
				{
					insertString += Format("%u,", uniformMap[u]);
				}
				insertString += Format("},\n");
			}

			if (!fixVertex.empty())
			{
				insertString += Format("		vsShader = [===[");
				insertString += Format("%s", fixVertex.c_str());
				insertString += Format("]===],\n");
			}

			if (!fixPixel.empty())
			{
				insertString += Format("		psShader = [===[");
				insertString += Format("%s", fixPixel.c_str());
				insertString += Format("]===],\n");
			}

			if (!fixCompute.empty())
			{
				insertString += Format("		csShader = [===[");
				insertString += Format("%s", fixCompute.c_str());
				insertString += Format("]===],\n");
			}

			insertString += Format("	},\n");
		}

		insertString += Format("}\n");

		// shader 替换
		finalShader.erase(beginPos, endPos - beginPos);
		finalShader.insert(beginPos, insertString);
	}

	// 处理params
	size_t beginToken = std::string::npos;
	size_t endToken = std::string::npos;
	const string beginparam = "#DEF_PARAMS";
	const string endparam = "#END_PARAMS";
	bool bfind = FindInTokensString(finalShader, beginparam,
		endparam, 0, beginToken, endToken);

	if (bfind)
	{
		size_t startPos = beginToken + beginparam.length();
		size_t strCount = endToken - startPos;
		std::string propertySeg = finalShader.substr(startPos, strCount);
		std::string propertyAttribute = ParsePropertyAttribute(propertySeg);
		std::string propertyContext = ParsePropertyContent(propertySeg);

		std::string property;
		property += Format("function DefineParams()\n");
		property += Format("%s", propertyContext.c_str());
		property += Format("%s", propertyAttribute.c_str());
		property += Format("\t\nend\n");

		finalShader.erase(beginToken, endToken - beginToken + endparam.length());
		finalShader.insert(beginToken, property);
	}

	// 处理pass
	vector<string> totalPassNames;
	const string beginPass = "#DEF_PASSES";
	const string endPass = "#END_PASSES";
	bfind = FindInTokensString(finalShader, beginPass,
		endPass, 0, beginToken, endToken);

	while (bfind)
	{
		finalShader.erase(endToken, endPass.length());
		finalShader.insert(endToken, "end");

		std::string passname = FindNextWord(finalShader, beginToken + beginPass.length() + 1);
		size_t passNamePos = FindTokenInText(finalShader, passname, beginToken + beginPass.length() + 1);
		size_t passNameEndPos = passNamePos + passname.length();
		std::string passDef;
		passDef = Format("function %s()\n", passname.c_str());
		finalShader.erase(beginToken, passNameEndPos - beginToken);
		finalShader.insert(beginToken, passDef);

		totalPassNames.push_back(passname);

		// 查找新的pass
		bfind = FindInTokensString(finalShader, beginPass,
			endPass, endToken, beginToken, endToken);
	}

	// 处理tag
	const string beginTag = "#DEFTAG";
	const string endTag = "#END";
	bfind = FindInTokensString(finalShader, beginTag,
		endTag, 0, beginToken, endToken);

	if (bfind)
	{
		finalShader.erase(endToken, endTag.length());
		finalShader.insert(endToken, "end");

		string initFunDef;
		initFunDef += Format("\nfunction Init()\n");
		initFunDef += Format("\tPassNames = \n");
		initFunDef += Format("\t{\n");
		initFunDef += Format("\t\t%s\n", Vector2String(totalPassNames).c_str());
		initFunDef += Format("\t}\n");
		initFunDef += Format("\nVersion = 1");	

		finalShader.erase(beginToken, beginTag.length());
		finalShader.insert(beginToken, initFunDef);
	}

	if (isCompute)
	{
		string computeFuncBegin = Format("function ComputePass()\n");
		string computerFuncEnd = Format("\nend\n");

		finalShader.insert(0, computeFuncBegin);
		finalShader += Format("\nVersion = 2");	// 之前的shader版本是1
		finalShader += computerFuncEnd;
	}

	return true;
}

bool CheckError(ComplierState& globalState)
{
	bool hasError = false;
	// Set locale based on current environment
	std::setlocale(LC_CTYPE, "");
	const vector<ShaderCollection>& shaderSet = globalState.shaderCollect;
	for (int i = 0; i < shaderSet.size(); i++)
	{
		const ShaderCollection& curOut = shaderSet[i];
		const std::vector<ShaderOut>& curCollect = curOut.shaderOuts;

		for (int j = 0; j < curCollect.size(); j++)
		{
			if (curCollect[j].error != "")
			{
				PrintError("-------------------shader complie failed-----------------------------------\n");
				PrintError("shader source:\n");
				PrintError(Format("%s", curCollect[j].unCompileShader.c_str()));
				PrintError("shader error:\n");
				PrintError(curCollect[j].error);
				hasError = true;
			}
		}
	}
	return hasError;
}
