#include "GLSLCompiler.h"
#include "hlslcc_lib/hlslcc_private.h"
#include "GlslBackend.h"
#include "ShaderPreprocessor.h"
#include "../utils.h"
#include "ShaderPreprocessor.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
	static std::unordered_map<std::string, Uniform> allUni =
	{
		{ "i1",{ "", "int", "0" } },
		{ "f1",{ "", "float", "0" } },
		{ "f2",{ "", "float2", "0" } },
		{ "f3",{ "", "float3", "0" } },
		{ "f4",{ "", "float4", "0" } },
		{ "f3x3",{ "", "float3x3", "0" } },
		{ "f4x4",{ "", "float4x4", "0" } },
		{ "t",{ "", "sampler2D", "0" } },
		{ "c",{ "", "samplerCube", "0" } },
		{ "im",{ "", "sampler2D", "1" } },
	};

	void StrParseUniform(const std::string& curline, const std::string& splitStr, std::vector<std::vector<std::string>>& strLists)
	{
		std::vector<std::string> curList = split(curline, splitStr);
		int count = curList.size();
		strLists.resize(count);

		for (int i = 0; i < count; i++)
		{
			std::vector<std::string> tmpList;
			std::vector<std::string>& curSubList = strLists[i];
			tmpList = split(curList[i], ";");

			if (tmpList.size() == 2)
			{
				curSubList.push_back(tmpList[0]);
				tmpList = split(tmpList[1], ":");

				if (tmpList.size() == 2)
				{
					curSubList.push_back(tmpList[0]);
					curSubList.push_back(tmpList[1]);
				}
			}
		}
	}

	std::vector<std::vector<std::string>> GetSplitItem(const std::string& shaderSource, std::string& prefix)
	{
		std::vector<std::vector<std::string>> strLists;

		size_t findPos = shaderSource.find(prefix);

		if (findPos != std::string::npos)
		{
			std::string curline = FindStringLine(shaderSource, findPos + prefix.size());
			StrParseUniform(curline, ",", strLists);
		}

		return strLists;
	}

	class FHlslccHeader
	{
	private:
		void parseStrToUninfo(Uniform& curUniform, const std::vector<std::string>& inSubList)
		{
			std::string type = inSubList[0];
			std::string strcnt("1");
			auto cntbpos = type.find('[');
			if (cntbpos != std::string::npos)
			{
				auto cntpos = type.find(']');
				strcnt = type.substr(cntbpos + 1, cntpos - cntbpos - 1);
				type = type.substr(0, cntbpos);
			}

			if (inSubList.size() == 3 && allUni.find(type) != allUni.end())
			{
				curUniform = allUni.find(type)->second;
				curUniform.name = inSubList[2];
				curUniform.num = strcnt;
				curUniform.regIndex = "0";
				curUniform.regCount = strcnt;
			}
			else
			{
				printf("parse uniform info failed!\n");
			}
		}
		void CalcateCBSize(Uniform& uni, const std::unordered_set<int>& cbuffMap);
		void ParseUniform(const std::string& ShaderSource);
		void ParseSampler(const std::string& ShaderSource);
		void ParseUAV(const std::string& ShaderSource);
		void ParseUniformBlocks(const std::string& ShaderSource);
	public:
		bool Read(const std::string& ShaderSource);
		void GetOpenglReflection(std::vector<Uniform>& uniforms);

		std::vector<Uniform> Uniforms;
		std::vector<Uniform> Samplers;
		std::vector<Uniform> UAVs;
	};

	void FHlslccHeader::CalcateCBSize(Uniform& uni, const std::unordered_set<int>& cbuffMap)
	{
		int total = 0;
		int used = 0;

		static std::unordered_map<std::string, std::tuple<int, int>> allUniInfo =
		{
			// type, {占用空间, 对齐数}
			{ "float", {4, 4} },
			{ "float2", {8, 8} },
			{ "float3", {12, 16} },
			{ "float4", {16, 16} },
			{ "float3x3", {48, 16} },
			{ "float4x4", {64, 16} },
		};

		auto fixMemory = [&](const std::string& type, int count) -> void	// 获取指定类型的首地址
		{
			std::tuple<int, int> info = allUniInfo[type];
			int alighNum = count > 1 ? 16 : std::get<1>(info);	//标量或向量的数组	每个元素的基准对齐量与vec4的相同。
			int temp = used;
			used = used / alighNum  * alighNum;
			if (used < temp)
			{
				used += alighNum;
			}
			//used = used + std::get<0>(info);
		};

		for (auto& uindex : cbuffMap)
		{
			Uniform& u = Uniforms[uindex];
			int count = atoi(u.regCount.c_str());
			fixMemory(u.type, count);

			std::tuple<int, int> info = allUniInfo[u.type];
			u.regIndex = std::to_string(used);
			used += count * std::get<0>(info);
		}
		int array_size = atoi(uni.num.c_str());
		int reg_count = atoi(uni.regCount.c_str());
		uni.regCount = std::to_string(reg_count + array_size * used);
	}

	void FHlslccHeader::ParseUniform(const std::string& ShaderSource)
	{
		std::string prefix = "// @Uniforms: ";
		std::vector<std::vector<std::string>> strLists = GetSplitItem(ShaderSource, prefix);
		int len = strLists.size();

		for (int i = 0; i < len; i++)
		{
			const std::vector<std::string>& subList = strLists[i];

			Uniform uni;
			parseStrToUninfo(uni, subList);
			Uniforms.push_back(std::move(uni));
		}
	}

	void FHlslccHeader::ParseSampler(const std::string& ShaderSource)
	{
		std::string prefix = "// @Samplers: ";
		std::vector<std::vector<std::string>> strLists = GetSplitItem(ShaderSource, prefix);
		int len = strLists.size();

		for (int i = 0; i < len; i++)
		{
			const std::vector<std::string>& subList = strLists[i];

			Uniform uni;
			parseStrToUninfo(uni, subList);
			// 修正采样器的索引
			uni.regIndex = '0' + i;
			Samplers.push_back(uni);
		}
	}

	void FHlslccHeader::ParseUAV(const std::string& ShaderSource)
	{
		std::string prefix = "// @UAVs: ";
		std::vector<std::vector<std::string>> strLists = GetSplitItem(ShaderSource, prefix);
		int len = strLists.size();

		int regIndex = 0;
		for (int i = 0; i < len; i++)
		{
			const std::vector<std::string>& subList = strLists[i];
			Uniform curUniform;

			assert(subList.size() == 3);

			if (allUni.find(subList[0]) != allUni.end())
			{
				parseStrToUninfo(curUniform, subList);
				// 修正采样器的索引
				curUniform.regIndex = '0' + regIndex;
				regIndex++;
				UAVs.push_back(curUniform);
			}
			else   // 异常情况，类型是buffer
			{
				curUniform.name = subList[2];
				curUniform.num = "1";
				curUniform.regIndex = subList[1];
				curUniform.regCount = "1";
				curUniform.sit = "2";
				curUniform.type = "buffer";

				UAVs.push_back(curUniform);
			}
		}
	}

	void FHlslccHeader::ParseUniformBlocks(const std::string& ShaderSource)
	{
		std::string prefix = "// @UniformBlocks: ";

		size_t findPos = ShaderSource.find(prefix);
		if (findPos != std::string::npos)
		{
			std::string curline = FindStringLine(ShaderSource, findPos + prefix.size());
			curline.pop_back();

			std::vector<std::string> curList = split(curline, "},");
			for (int i = 0; i < curList.size(); ++i)
			{
				Uniforms.emplace_back();
				auto& cbuffer = Uniforms.back();

				size_t lxk = curList[i].find("(");
				size_t rxk = curList[i].find(")");
				size_t ldk = curList[i].find("{");

				cbuffer.name = curList[i].substr(0, lxk);
				cbuffer.type = "constBuffer";
				cbuffer.num = std::to_string(1);
				cbuffer.regIndex = curList[i].substr(lxk + 1, rxk - lxk - 1).c_str();

				std::string uniforminfo = curList[i].substr(ldk + 1);
				std::string strsStructUni;
				auto structPos = uniforminfo.find('#');
				if (structPos != std::string::npos)
				{
					strsStructUni = uniforminfo.substr(structPos + 1);
					uniforminfo = uniforminfo.substr(0, structPos);
				}

				std::vector<std::vector<std::string>> strLists;
				StrParseUniform(uniforminfo, ",", strLists);

				std::unordered_set<int> subVal;
				int cbindex = Uniforms.size() - 1;
				for (auto& uniInfos : strLists)
				{
					if (!strsStructUni.empty())
					{
						// 解析出数组大小
						auto cntbpos = uniInfos[0].find('[');
						if (cntbpos != std::string::npos)
						{
							auto cntpos = uniInfos[0].find(']');
							cbuffer.num = uniInfos[0].substr(cntbpos + 1, cntpos - cntbpos - 1);
						}

						std::vector<std::vector<std::string>> subStrLists;
						StrParseUniform(strsStructUni, "|", subStrLists);
						for (auto& subStrUni : subStrLists)
						{
							subVal.insert(Uniforms.size());
							Uniforms.emplace_back();
							Uniform& structuni = Uniforms.back();
							parseStrToUninfo(structuni, subStrUni);
						}
					}
					else
					{
						int subCBIndex = Uniforms.size();
						subVal.insert(subCBIndex);
						Uniforms.emplace_back();
						Uniform& uni = Uniforms.back();
						parseStrToUninfo(uni, uniInfos);
					}
				}
				CalcateCBSize(Uniforms[cbindex], subVal);

				for (auto idx : subVal)
				{
					Uniforms[cbindex].subIndexs.push_back(std::hash<Uniform>{}(Uniforms[idx]));
				}
			}
		}
	}

	bool FHlslccHeader::Read(const std::string& ShaderSource)
	{
		ParseUniform(ShaderSource);
		ParseSampler(ShaderSource);
		ParseUAV(ShaderSource);
		ParseUniformBlocks(ShaderSource);

		return true;
	}

	void FHlslccHeader::GetOpenglReflection(std::vector<Uniform>& uniforms)
	{
		uniforms.insert(uniforms.end(), Uniforms.begin(), Uniforms.end());
		uniforms.insert(uniforms.end(), Samplers.begin(), Samplers.end());
		uniforms.insert(uniforms.end(), UAVs.begin(), UAVs.end());
	}

	void RemoveShaderComment(std::string& shadercontext)
	{
		auto pos = shadercontext.find("#version");
		shadercontext = shadercontext.substr(pos);
	}
}

EHlslCompileTarget GetShaderLevel(ShaderAPI api, bool hasComputeShader)
{
	EHlslCompileTarget shaderLevel = HCT_InvalidTarget;

	switch (api)
	{
	case OPENGL:
		shaderLevel = HCT_FeatureLevelSM5;
		break;
	case GLES2:
		shaderLevel = HCT_FeatureLevelES2;
		break;
	case GLES31:
	{
		if (hasComputeShader)
		{
			shaderLevel = HCT_FeatureLevelES3_1Ext;
		}
		else
		{
			shaderLevel = HCT_FeatureLevelES3_1;
		}
		break;
	}
	case Metal:
		shaderLevel = HCT_FeatureLevelES3_1Ext;
		break;
	default:
		break;
	}

	return shaderLevel;
}

class ShaderItem
{
public:
	std::string* shaderName{};
	std::string* shaderOutFile{};
	std::vector<Uniform>* shaderOutUniform{};
};


bool CrossComplie(ShaderAPI api, ComplierState& gloablState)
{
	int Flags = 0;
	Flags |= HLSLCC_NoValidation;

	bool hasComputerShader = (!gloablState.active.param.csName.empty());
	EHlslCompileTarget shaderLevel = GetShaderLevel(api, hasComputerShader);
	FCodeBackend* CodeBackend = nullptr;
	ILanguageSpec* LanguageSpec = nullptr;


	bool bInIsES2 = (EHlslCompileTarget::HCT_FeatureLevelES2 == shaderLevel);
	bool bInIsES31 = (EHlslCompileTarget::HCT_FeatureLevelES3_1 == shaderLevel);

	std::vector<std::string>& curSysKeyWords = gloablState.active.curOut.systemKeyWords;

	CodeBackend = new FGlslCodeBackend(Flags, shaderLevel, false);
	LanguageSpec = new FGlslLanguageSpec(bInIsES2, false, bInIsES31);

	if (api == Metal)
	{
		curSysKeyWords.push_back("COMPILER_METAL");
	}
	else if (api == ShaderAPI::OPENGL)
	{
		// opengl
		curSysKeyWords.push_back("COMPILER_GLSL");
	}
	else if (api == ShaderAPI::GLES2)
	{
		// gles2
		curSysKeyWords.push_back("COMPILER_GLSL_ES2");
	}
	else if (api == ShaderAPI::GLES31)
	{
		// gles31
		curSysKeyWords.push_back("COMPILER_GLSL_ES3_1");
	}

	gloablState.active.curOut.unCompileShader = gloablState.active.shaderSeg;
	std::string& shaderSource = gloablState.active.curOut.unCompileShader;
	ReplaceString(shaderSource, "#pragma", "// #pragma");

	char* HLSLShaderSource = nullptr;
	char* finalShaderSource = nullptr;
	char* ErrorLog = nullptr;

	// 编译 vs ps cs
	std::vector<ShaderItem> shaderEntrys;
	shaderEntrys.push_back({ &gloablState.active.param.vsName, &gloablState.active.curOut.compileVShader, &gloablState.active.curOut.vsUniforms });
	shaderEntrys.push_back({ &gloablState.active.param.psName, &gloablState.active.curOut.compilePShader, &gloablState.active.curOut.psUniforms });
	shaderEntrys.push_back({ nullptr, nullptr, nullptr });
	shaderEntrys.push_back({ nullptr, nullptr, nullptr });
	shaderEntrys.push_back({ nullptr, nullptr, nullptr });
	shaderEntrys.push_back({ &gloablState.active.param.csName, &gloablState.active.curOut.compileCShader, &gloablState.active.curOut.csUniforms });

	
	bool hasError = false;
	for (int32 Index = HSF_VertexShader; Index < HSF_FrequencyCount; ++Index)
	{
		ShaderProcesser processer(gloablState);
		bool preprocessSucess = processer.Run();
		if (!preprocessSucess)
		{
			return false;
		}

		ShaderItem& curItem = shaderEntrys[Index];
		if (curItem.shaderName == nullptr || curItem.shaderName->empty())
		{
			continue;
		}
		std::string& pShaderName = *curItem.shaderName;

		if (api != Metal)
		{
			FHlslCrossCompilerContext Context(Flags, (EHlslShaderFrequency)Index, shaderLevel);
			if (Context.Init("curShader", LanguageSpec))
			{
				int result = Context.Run(
					shaderSource.c_str(),
					pShaderName.c_str(),
					CodeBackend,
					&finalShaderSource,
					&ErrorLog) ? 1 : 0;

				if (!result || !finalShaderSource)
				{
					if (ErrorLog)
					{
						gloablState.active.curOut.error = Format("compiler shader = %s has error \n, error Info %s \n", pShaderName.c_str(), ErrorLog);
						free(ErrorLog);
					}
					else
					{
						gloablState.active.curOut.error = Format("compiler shader = %s has error \n", pShaderName.c_str());
					}
					gloablState.active.curOut.unCompileShader = gloablState.active.shaderSeg;
					hasError = true;
					break;
				}

				if (ErrorLog)
				{
					// 这里产生的是警告信息
					free(ErrorLog);
				}

				FHlslccHeader head;
				head.Read(finalShaderSource);
				head.GetOpenglReflection(*curItem.shaderOutUniform);

				*curItem.shaderOutFile = finalShaderSource;
				RemoveShaderComment(*curItem.shaderOutFile);

				free(finalShaderSource);
			}
			else
			{
				gloablState.active.curOut.error = Format("compiler shader = %s has error", pShaderName.c_str());
				hasError = true;
				break;
			}
		}
		else
		{
			// TODO metal
		}
	}
	
	return !hasError;
}