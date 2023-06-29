/*
To do: line 587, include header and macro defines
*/

#include <cassert>
#include <algorithm>

#include "hlslCompiler.h"
//#include "../mojoshader/mojoshader.h"
#include "../Utils.h"
#include "../ByteArrayEncoder.h"
#include "../glsl/ShaderPreprocessor.h"

using namespace std;

// below is code from bgfx
#ifndef D3D_SVF_USED
#	define D3D_SVF_USED 2
#endif // D3D_SVF_USED

#define BGFX_UNIFORM_FRAGMENTBIT UINT8_C(0x10)
#define BGFX_UNIFORM_SAMPLERBIT  UINT8_C(0x20)


#define DEBUGMODE


const unsigned kMaxFilePath = 1024;

namespace hlsl
{
	std::vector<Attribute> GetAttrInfo(const commattri::Attrib::Enum* in_attrs, uint32_t size);
	std::string GetAttributeTypeStr(commattri::Attrib::Enum _type);
	std::string StripUnUsedUniforms(std::string& shaderCode, std::vector<std::string>& unUsednames);

	typedef HRESULT(WINAPI* PFN_D3D_COMPILE)(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData
		, _In_ SIZE_T SrcDataSize
		, _In_opt_ LPCSTR pSourceName
		, _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines
		, _In_opt_ ID3DInclude* pInclude
		, _In_opt_ LPCSTR pEntrypoint
		, _In_ LPCSTR pTarget
		, _In_ UINT Flags1
		, _In_ UINT Flags2
		, _Out_ ID3DBlob** ppCode
		, _Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorMsgs
		);

	typedef HRESULT(WINAPI* PFN_D3D_DISASSEMBLE)(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData
		, _In_ SIZE_T SrcDataSize
		, _In_ UINT Flags
		, _In_opt_ LPCSTR szComments
		, _Out_ ID3DBlob** ppDisassembly
		);

	typedef HRESULT(WINAPI* PFN_D3D_REFLECT)(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData
		, _In_ SIZE_T SrcDataSize
		, _In_ REFIID pInterface
		, _Out_ void** ppReflector
		);

	typedef HRESULT(WINAPI* PFN_D3D_STRIP_SHADER)(_In_reads_bytes_(BytecodeLength) LPCVOID pShaderBytecode
		, _In_ SIZE_T BytecodeLength
		, _In_ UINT uStripFlags
		, _Out_ ID3DBlob** ppStrippedBlob
		);

	PFN_D3D_COMPILE      D3DCompile;
	PFN_D3D_DISASSEMBLE  D3DDisassemble;
	PFN_D3D_REFLECT      D3DReflect;
	PFN_D3D_STRIP_SHADER D3DStripShader;

	struct D3DCompiler
	{
		const char* fileName;
		const GUID  IID_ID3D11ShaderReflection;
	};

	static const D3DCompiler s_d3dcompiler[] =
	{ // BK - the only different method in interface is GetRequiresFlags at the end
	  //      of IID_ID3D11ShaderReflection47 (which is not used anyway).
		{ "D3DCompiler_47.dll",{ 0x8d536ca1, 0x0cca, 0x4956,{ 0xa8, 0x37, 0x78, 0x69, 0x63, 0x75, 0x55, 0x84 } } },
		{ "D3DCompiler_46.dll",{ 0x0a233719, 0x3960, 0x4578,{ 0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1 } } },
		{ "D3DCompiler_45.dll",{ 0x0a233719, 0x3960, 0x4578,{ 0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1 } } },
		{ "D3DCompiler_44.dll",{ 0x0a233719, 0x3960, 0x4578,{ 0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1 } } },
		{ "D3DCompiler_43.dll",{ 0x0a233719, 0x3960, 0x4578,{ 0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1 } } },
	};

	static const D3DCompiler* s_compiler;
	static void* s_d3dcompilerdll;

	void unload()
	{
		::FreeLibrary((HMODULE)s_d3dcompilerdll);
	}

	const D3DCompiler* load()
	{
		for (uint32_t ii = 0; ii < BX_COUNTOF(s_d3dcompiler); ++ii)
		{
			const D3DCompiler* compiler = &s_d3dcompiler[ii];
			s_d3dcompilerdll = LoadLibraryA(compiler->fileName);
			if (NULL == s_d3dcompilerdll)
			{
				continue;
			}

			D3DCompile = (PFN_D3D_COMPILE)GetProcAddress((HMODULE)s_d3dcompilerdll, "D3DCompile");
			D3DDisassemble = (PFN_D3D_DISASSEMBLE)GetProcAddress((HMODULE)s_d3dcompilerdll, "D3DDisassemble");
			D3DReflect = (PFN_D3D_REFLECT)GetProcAddress((HMODULE)s_d3dcompilerdll, "D3DReflect");
			D3DStripShader = (PFN_D3D_STRIP_SHADER)GetProcAddress((HMODULE)s_d3dcompilerdll, "D3DStripShader");

			if (NULL == D3DCompile
				|| NULL == D3DDisassemble
				|| NULL == D3DReflect
				|| NULL == D3DStripShader)
			{
				unload();
				continue;
			}

#ifdef DEBUGMODE
			char filePath[kMaxFilePath];
			GetModuleFileNameA((HMODULE)s_d3dcompilerdll, filePath, sizeof(filePath));
#endif // DEBUGMODE

			return compiler;
		}

		fprintf(stderr, "Error: Unable to open D3DCompiler_*.dll shader compiler.\n");
		return NULL;
	}


	std::string findUniformType(const D3D11_SHADER_TYPE_DESC& constDesc)
	{
		for (uint32_t ii = 0; ii < BX_COUNTOF(s_uniformRemap); ++ii)
		{
			const UniformRemap& remap = s_uniformRemap[ii];

			if (remap.paramClass == constDesc.Class
				&&  remap.paramType == constDesc.Type
				&& remap.columns == constDesc.Columns
				&& remap.rows == constDesc.Rows)
			{
				return remap.id;
			}
		}

		return "";
	}

	static uint32_t s_optimizationLevelD3D11[4] =
	{
		D3DCOMPILE_OPTIMIZATION_LEVEL0,
		D3DCOMPILE_OPTIMIZATION_LEVEL1,
		D3DCOMPILE_OPTIMIZATION_LEVEL2,
		D3DCOMPILE_OPTIMIZATION_LEVEL3,
	};

	typedef std::vector<std::string> UniformNameList;

	static bool isSampler(D3D_SHADER_VARIABLE_TYPE _svt)
	{
		switch (_svt)
		{
		case D3D_SVT_SAMPLER:
		case D3D_SVT_SAMPLER1D:
		case D3D_SVT_SAMPLER2D:
		case D3D_SVT_SAMPLER3D:
		case D3D_SVT_SAMPLERCUBE:
			return true;

		default:
			break;
		}

		return false;
	}

	static std::string getType(D3D_SRV_DIMENSION _svt)
	{
		std::string texType = "";
		switch (_svt)
		{
		case D3D_SRV_DIMENSION_TEXTURE1D:
			texType= "sampler1D";
			break;
		case D3D_SRV_DIMENSION_TEXTURE2D:
			texType = "sampler2D";
			break;
		case D3D_SRV_DIMENSION_TEXTURE3D:
			texType = "sampler3D";
			break;
		case D3D_SRV_DIMENSION_TEXTURECUBE:
			texType = "samplerCube";
			break;
		case D3D_SRV_DIMENSION_BUFFER:
			texType = "buffer";
			break;
		default:
			break;
		}

		return texType;
	}

	static std::string getInputType(D3D_SHADER_INPUT_TYPE sit)
	{
		std::string sType = "0";
		switch (sit)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			sType = "0";  // read srv
			break;
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
			sType = "1"; // write uav
			break;
		default:
			assert(0);
			break;
		}
		return sType;
	}

	bool getReflectionDataD3D11(ID3DBlob* _code, bool _vshader, std::vector<Uniform>& _uniforms, uint8_t& _numAttrs,
		commattri::Attrib::Enum* _attrs, uint16_t& _size, std::vector<std::string>& unusedUniforms)
	{
		ID3D11ShaderReflection* reflect = NULL;
		HRESULT hr = D3DReflect(_code->GetBufferPointer()
			, _code->GetBufferSize()
			, s_compiler->IID_ID3D11ShaderReflection
			, (void**)&reflect
		);
		if (FAILED(hr))
		{
			return false;
		}

		D3D11_SHADER_DESC desc;
		hr = reflect->GetDesc(&desc);
		if (FAILED(hr))
		{
			return false;
		}

		if (_vshader) // Only care about input semantic on vertex shaders
		{
			for (uint32_t ii = 0; ii < desc.InputParameters; ++ii)
			{
				D3D11_SIGNATURE_PARAMETER_DESC spd;
				reflect->GetInputParameterDesc(ii, &spd);
				if (std::strcmp(spd.SemanticName, "BITANGENT") == 0)
				{
					PrintError("Warning: Attribute BITANGENT would conflict with BINORMAL in D3D");
				}

				const commattri::RemapInputSemantic& ris = commattri::findInputSemantic(spd.SemanticName, uint8_t(spd.SemanticIndex));
				if (ris.m_attr != commattri::Attrib::Count)
				{
					_attrs[_numAttrs] = ris.m_attr;
					++_numAttrs;
				}
			}
		}

		for (uint32_t ii = 0; ii < desc.OutputParameters; ++ii)
		{
			D3D11_SIGNATURE_PARAMETER_DESC spd;
			reflect->GetOutputParameterDesc(ii, &spd);
		}

		for (uint32_t ii = 0, num = desc.ConstantBuffers; ii < num; ++ii)
		{
			ID3D11ShaderReflectionConstantBuffer* cbuffer = reflect->GetConstantBufferByIndex(ii);
			D3D11_SHADER_BUFFER_DESC bufferDesc;
			hr = cbuffer->GetDesc(&bufferDesc);

			if (bufferDesc.Type != D3D_CT_CBUFFER)
			{
				continue;
			}

			int cbIndx = 0;
			bool bConstBuffer = strcmp(std::string(bufferDesc.Name).c_str(), "$Globals") != 0;
			if (bConstBuffer)
			{
				Uniform un;
				un.name = bufferDesc.Name;
				un.type = "constBuffer";
				un.num = std::to_string(bufferDesc.Variables);
				un.regIndex = std::to_string(0);
				un.regCount = std::to_string(bufferDesc.Size);

				cbIndx = _uniforms.size();
				_uniforms.push_back(un);
			}

			_size = (uint16_t)bufferDesc.Size;

			if (SUCCEEDED(hr))
			{
				for (uint32_t jj = 0; jj < bufferDesc.Variables; ++jj)
				{
					ID3D11ShaderReflectionVariable* var = cbuffer->GetVariableByIndex(jj);
					ID3D11ShaderReflectionType* type = var->GetType();
					D3D11_SHADER_VARIABLE_DESC varDesc;
					hr = var->GetDesc(&varDesc);
					if (SUCCEEDED(hr))
					{
						D3D11_SHADER_TYPE_DESC constDesc;
						hr = type->GetDesc(&constDesc);
						if (SUCCEEDED(hr))
						{
							std::string uniformType = findUniformType(constDesc);

							// 加入一个uniform，返回该对象在容器中的索引
							auto processUniform = [&]() -> int
							{
								int elementCount = (constDesc.Elements == 0 ? 1 : constDesc.Elements);
								Uniform un;
								un.name = varDesc.Name;
								un.type = uniformType;
								un.num = std::to_string(elementCount);
								un.regIndex = std::to_string(varDesc.StartOffset);
								un.regCount = std::to_string(BX_ALIGN_MASK(varDesc.Size, 0xf) / 16);
								_uniforms.push_back(un);
								return _uniforms.size() - 1;
							};

							if (bConstBuffer)
							{
								if ("" != uniformType && 0 != (varDesc.uFlags & D3D_SVF_USED))
								{
									int uindex = processUniform();
									_uniforms[cbIndx].subIndexs.push_back(std::hash<Uniform>{}(_uniforms[uindex]));
								}
								else if (bConstBuffer && constDesc.Class == D3D_SVC_STRUCT)
								{
									_uniforms[cbIndx].num = std::to_string(constDesc.Elements);
									for (int subIdx = 0; subIdx < constDesc.Members; ++subIdx)
									{
										ID3D11ShaderReflectionType* subType = type->GetMemberTypeByIndex(subIdx);
										D3D11_SHADER_TYPE_DESC subconstDesc;
										assert(SUCCEEDED(subType->GetDesc(&subconstDesc)));
										std::string subUniformType = findUniformType(subconstDesc);
										if (!subUniformType.empty())
										{
											int elementCount = (subconstDesc.Elements == 0 ? 1 : subconstDesc.Elements);
											Uniform un;
											un.name = type->GetMemberTypeName(subIdx);
											un.type = uniformType;
											un.num = std::to_string(elementCount);
											un.regIndex = std::to_string(subconstDesc.Offset);
											un.regCount = std::to_string(elementCount);

											_uniforms.push_back(un);

											if (bConstBuffer)
											{
												_uniforms[cbIndx].subIndexs.push_back(std::hash<Uniform>{}(un));
											}
										}
									}
								}
								else
								{
									if (0 == (varDesc.uFlags & D3D_SVF_USED))
									{
										unusedUniforms.push_back(varDesc.Name);
									}
								}
							}
							else
							{
								if ("" != uniformType && 0 != (varDesc.uFlags & D3D_SVF_USED))
								{
									processUniform();
								}
								else
								{
									if (0 == (varDesc.uFlags & D3D_SVF_USED))
									{
										unusedUniforms.push_back(varDesc.Name);
									}
								}
							}
						}
					}
				}
			}
		}

		for (uint32_t ii = 0; ii < desc.BoundResources; ++ii)
		{
			D3D11_SHADER_INPUT_BIND_DESC bindDesc;

			hr = reflect->GetResourceBindingDesc(ii, &bindDesc);
			if (SUCCEEDED(hr))
			{
				if (bindDesc.Type != D3D_SIT_CBUFFER && bindDesc.Type != D3D_SIT_SAMPLER)
				{
					Uniform un;
					un.name = bindDesc.Name;
					un.type = getType(bindDesc.Dimension);
					un.sit = getInputType(bindDesc.Type);
					un.num = "1";
					un.regIndex = std::to_string(bindDesc.BindPoint);
					un.regCount = "1";
					_uniforms.push_back(un);
				}
			}
		}

		if (NULL != reflect)
		{
			reflect->Release();
		}

		return true;
	}

	struct IncludeHandler : ID3DInclude
	{
		ComplierState* m_Data;
		std::vector<std::string> unUsedUniforms;

		virtual HRESULT __stdcall Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
		{
			for (size_t i = 0; i < m_Data->includePaths.size(); ++i)
			{
				string fullFileName = m_Data->includePaths[i] + "/" + pFileName;
				std::string str;
				if (ReadFileContent(fullFileName, str))
				{
					// 去除没有使用的变量
					// 目前只主处理变量最多的头文件
					if (unUsedUniforms.size() > 0
						&& strcmp("commonvar.inc", pFileName) == 0)
					{
						str = StripUnUsedUniforms(str, unUsedUniforms);
					}

					int len = str.length();
					char* data = new char[len];
					memcpy_s(data, len, str.c_str(), len);

					*ppData = data;
					*pBytes = len;

					return S_OK;
				}
			}

			return S_FALSE;
		}

		virtual HRESULT __stdcall Close(LPCVOID pData)
		{
			delete[](char*)pData;
			return S_OK;
		}
	};


	void InitCompiler()
	{
		s_compiler = load();
	}

	void UnInitCompiler()
	{
		unload();
	}

	bool compile(ComplierState& gloablState, ShaderStage stage, bool debug, bool onlyCheck, std::vector<string>& unUsedUniforms)
	{
		bool result = false;

		uint32_t flags = 0;

		if (debug)
		{
			flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION
				| D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
		}
		else
		{
			flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
		}

		std::string profile;
		const char* pCode = nullptr;
		const char* pEntryName = nullptr;
		ShaderCollection& collection = gloablState.active;
		ShaderOut& curOut = collection.curOut;
		string& error = curOut.error;
		size_t codeSize = 0;
		std::vector<Uniform>* pUniforms = nullptr;
		std::vector<Attribute>* pAttributes = nullptr;
		std::string* pOutShader = nullptr;
		std::string* pConstantSz = nullptr;

		if (stage == VERTEX_SHADER_Stage)
		{
			profile = std::string("vs_5_0");
			pCode = curOut.unCompileShader.c_str();
			codeSize = curOut.unCompileShader.length();
			pEntryName = collection.param.vsName.c_str();
			pUniforms = &(curOut.vsUniforms);
			pOutShader = &(curOut.compileVShader);
			pAttributes = &(curOut.vsAttributes);
			pConstantSz = &(curOut.vsConstantSize);
		}
		else if (stage == PIXEL_SHADER_Stage)
		{
			profile = std::string("ps_5_0");
			pCode = curOut.unCompileShader.c_str();
			codeSize = curOut.unCompileShader.length();
			pEntryName = collection.param.psName.c_str();
			pUniforms = &(curOut.psUniforms);
			pOutShader = &(curOut.compilePShader);
			pAttributes = nullptr;
			pConstantSz = &(curOut.psConstantSize);
		}
		else if (stage == COMPUTE_SHADER_Stage)
		{
			profile = std::string("cs_5_0");
			pCode = curOut.unCompileShader.c_str();
			codeSize = curOut.unCompileShader.length();
			pEntryName = collection.param.csName.c_str();
			pUniforms = &(curOut.csUniforms);
			pOutShader = &(curOut.compileCShader);
			pAttributes = nullptr;
			pConstantSz = &(curOut.csConstantSize);
		}

		ID3DBlob* code;
		ID3DBlob* errorMsg;
		IncludeHandler includeHander;
		includeHander.m_Data = &gloablState;

		if (!onlyCheck)
		{
			includeHander.unUsedUniforms = unUsedUniforms;
		}

		HRESULT hr = D3DCompile(pCode
			, codeSize
			, NULL
			, NULL
			, &includeHander
			, pEntryName
			, profile.c_str()
			, flags
			, 0
			, &code
			, &errorMsg
		);

		if (FAILED(hr))
		{
			const char* log = (char*)errorMsg->GetBufferPointer();

			int32_t line = 0;
			int32_t column = 0;
			int32_t start = 0;
			int32_t end = INT32_MAX;

			bool found = false
				|| 2 == sscanf(log, "(%u,%u", &line, &column)
				|| 2 == sscanf(log, " :%u:%u: ", &line, &column)
				;

			if (found
				&& 0 != line)
			{
				start = std::max<uint32_t>(1, line - 10);
				end = start + 20;
			}

			error += Format("Error: D3DCompile failed 0x%08x %s\n", (uint32_t)hr, log);
			errorMsg->Release();
			if (code != nullptr)
			{
				code->Release();
			}
			return false;
		}

		std::vector<Uniform> uniforms;
		uint8_t numAttrs = 0;
		commattri::Attrib::Enum attrs[commattri::Attrib::Count];
		uint16_t size = 0;

		std::vector<std::string> unUsed;
		if (!getReflectionDataD3D11(code, stage == VERTEX_SHADER_Stage, uniforms, numAttrs, attrs, size, unUsed))
		{
			error += Format("Error: Unable to get D3D11 reflection data.\n");
			code->Release();
			return false;
		}

		if (onlyCheck)
		{
			unUsedUniforms.clear();
			unUsedUniforms = unUsed;
			if (code != nullptr)
			{
				code->Release();
			}
		}
		else
		{
			ID3DBlob* stripped;
			hr = D3DStripShader(code->GetBufferPointer()
				, code->GetBufferSize()
				, D3DCOMPILER_STRIP_REFLECTION_DATA
				| D3DCOMPILER_STRIP_TEST_BLOBS
				, &stripped
			);
			if (code != nullptr)
			{
				code->Release();
			}

			if (SUCCEEDED(hr))
			{
				code->Release();
				code = stripped;

				// output shader info
				*pUniforms = uniforms;

				uint32_t shaderSize = uint32_t(code->GetBufferSize());
				const char* data = (const char*)code->GetBufferPointer();

				std::vector<byte> shaderInByte;
				shaderInByte.clear();
				shaderInByte.reserve(shaderSize);
				shaderInByte.insert(shaderInByte.end(), data, data + shaderSize);

				*pOutShader = ByteArrayEncoder::EncodeShader(shaderInByte);

				if (pAttributes != nullptr)
				{
					*pAttributes = GetAttrInfo(attrs, numAttrs);
				}
				*pConstantSz = std::to_string(size);
				stripped->Release();
			}
			else
			{
				error += Format("strip failed");
				if (stripped != nullptr)
				{
					stripped->Release();
				}
				return false;
			}
		}

		return true;
	}

	static char* hlslTypes[] =
	{
		"fixed",
		"fixed2",
		"fixed3",
		"fixed4",
		"half",
		"half2",
		"half3",
		"half4",
		"float",
		"float2",
		"float3",
		"float4",
		"float3x3",
		"float4x4",
		"sampler2D",
		"samplerCube",
	};
	
	bool isType(std::string& word)
	{
		bool result = false;
		for (int i = 0; i < sizeof(hlslTypes) / sizeof(char*); i++)
		{
			if (word == hlslTypes[i])
			{
				result = true;
				break;
			}
		}
		return result;
	}

	std::string StripUnUsedUniforms(std::string& shaderCode, std::vector<std::string>& unUsednames)
	{
		std::string output;
		size_t bgeinPos = 0;
		size_t findPos = shaderCode.find(';', bgeinPos);
		std::vector<Uniform> uniforms;

		while (findPos != std::string::npos)
		{
			std::string curStr = shaderCode.substr(bgeinPos, findPos - bgeinPos + 1);
			const char* pChar = curStr.c_str();

			for (std::vector<std::string>::iterator it = unUsednames.begin(), itEnd = unUsednames.end(); it != itEnd; ++it)
			{
				size_t index = curStr.find(*it);
				if (index == std::string::npos)
				{
					continue;
				}

				int strLen = (*it).size();
				char pNextChar = pChar[index + strLen];

				if (pNextChar == ';' || isspace(pNextChar))
				{
					std::string  preWord = FindPreWord(curStr, index - 1);
					if (isType(preWord))
					{
						if (curStr.find("uniform") != string::npos)
						{
							ReplaceString(curStr, "uniform", "static");
						}
						else if (curStr.find("static") == string::npos)
						{
							std::string replaceStr = "static\t" + preWord;
							ReplaceString(curStr, preWord, replaceStr);
						}

						break;
					}
				}
			}

			output += curStr;

			bgeinPos = findPos + 1;
			findPos = shaderCode.find(';', bgeinPos);
		}

		if (findPos == std::string::npos)
		{
			output += shaderCode.substr(bgeinPos);
		}

		return output;
	}

	bool ComplieHLSLShaderInternal(ComplierState& gloablState)
	{
		InitCompiler();

		ShaderCollection& collection = gloablState.active;
		ShaderOut& curOut = collection.curOut;
		string& error = curOut.error;
		std::string shaderCode = curOut.unCompileShader;

		bool debug = false;
		
		std::vector<std::string> unUsedUnifroms;
		bool result;

		// vs shader compile
		if (!gloablState.active.param.vsName.empty())
		{
			bool result = compile(gloablState, VERTEX_SHADER_Stage, debug, true, unUsedUnifroms);
			if (!result)
			{
				error += Format("compile Vs failed when in first pass");
				return false;
			}

			result = compile(gloablState, VERTEX_SHADER_Stage, debug, false, unUsedUnifroms);
			if (!result)
			{
				error += Format("compile Vs failed when in second pass");
				return false;
			}
		}

		// ps shader compile
		if (!gloablState.active.param.psName.empty())
		{
			curOut.unCompileShader = shaderCode;
			result = compile(gloablState, PIXEL_SHADER_Stage, debug, true, unUsedUnifroms);
			if (!result)
			{
				error += Format("compile ps failed when in first pass");
				return false;
			}

			result = compile(gloablState, PIXEL_SHADER_Stage, debug, false, unUsedUnifroms);
			if (!result)
			{
				error += Format("compile ps failed when in second pass");
				return false;
			}
			// reset shader code
			curOut.unCompileShader = shaderCode;
		}

		// cs shader compile
		if (!gloablState.active.param.csName.empty())
		{
			curOut.unCompileShader = shaderCode;
			result = compile(gloablState, COMPUTE_SHADER_Stage, debug, true, unUsedUnifroms);

			if (!result)
			{
				error += Format("compile cs failed");
				return false;
			}

			result = compile(gloablState, COMPUTE_SHADER_Stage, debug, false, unUsedUnifroms);
			if (!result)
			{
				error += Format("compile cs failed when in second pass");
				return false;
			}
			// reset shader code
			curOut.unCompileShader = shaderCode;
		}

		UnInitCompiler();
		return true;
	}

	std::vector<Attribute> GetAttrInfo(const commattri::Attrib::Enum* in_attrs, uint32_t size)
	{
		std::vector<Attribute> attrs;
		attrs.reserve(size);

		for (int i = 0; i < size; ++i)
		{
			Attribute att;
			att.attName = commattri::GetAttributeTypeStr(in_attrs[i]);
			att.attID = std::to_string(commattri::attribToId(in_attrs[i]));
			attrs.push_back(att);
		}
		return attrs;
	}

} // namespace hlsl

bool ComplieHLSLShader(ComplierState& gloablState)
{
	ShaderOut& out = gloablState.active.curOut;
	out.unCompileShader = gloablState.active.shaderSeg;
	std::string& shaderSource = out.unCompileShader;
	vector<string>& keyWorlds = out.keyWords;
	ReplaceString(shaderSource, "#pragma", "// #pragma");

	std::vector<std::string>& curSysKeyWords = gloablState.active.curOut.systemKeyWords;
	curSysKeyWords.push_back("COMPILER_HLSL");

	// 引入新的shader的预处理代码
	bool sucess = false;
	ShaderProcesser* processer = new ShaderProcesser(gloablState);
	bool preprocessSucess = processer->Run();
	delete processer;

	if (preprocessSucess)
	{
		bool result = hlsl::ComplieHLSLShaderInternal(gloablState);

		if (result)
		{
			sucess = true;
		}
	}
	
	return sucess;
}
