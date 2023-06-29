#pragma once
#include <limits.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include "../Macros.h"
#include "../ComplierState.h"
#include "../CommAttri.h"

#include <d3dcompiler.h>
#include <d3d11shader.h>

namespace hlsl
{

	struct CTHeader
	{
		uint32_t Size;
		uint32_t Creator;
		uint32_t Version;
		uint32_t Constants;
		uint32_t ConstantInfo;
		uint32_t Flags;
		uint32_t Target;
	};

	struct CTInfo
	{
		uint32_t Name;
		uint16_t RegisterSet;
		uint16_t RegisterIndex;
		uint16_t RegisterCount;
		uint16_t Reserved;
		uint32_t TypeInfo;
		uint32_t DefaultValue;
	};

	struct CTType
	{
		uint16_t Class;
		uint16_t Type;
		uint16_t Rows;
		uint16_t Columns;
		uint16_t Elements;
		uint16_t StructMembers;
		uint32_t StructMemberInfo;
	};

	struct UniformRemap
	{
		std::string id;
		D3D_SHADER_VARIABLE_CLASS paramClass;
		D3D_SHADER_VARIABLE_TYPE paramType;
		uint8_t columns;
		uint8_t rows;
	};

	static const UniformRemap s_uniformRemap[] =
	{
		{ "sampler2D", D3D_SVC_SCALAR,         D3D_SVT_INT,         1, 1 },
		{ "float",  D3D_SVC_SCALAR,         D3D_SVT_FLOAT,       1, 1 },
		{ "float2",  D3D_SVC_VECTOR,         D3D_SVT_FLOAT,       2, 1 },
		{ "float3",  D3D_SVC_VECTOR,         D3D_SVT_FLOAT,       3, 1 },
		{ "float4",  D3D_SVC_VECTOR,         D3D_SVT_FLOAT,       4, 1 },
		{ "float3x3", D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT,       3, 3 },
		{ "float4x4", D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT,       4, 4 },
		{ "float3x3", D3D_SVC_MATRIX_ROWS, D3D_SVT_FLOAT,       3, 3 },
		{ "float4x4", D3D_SVC_MATRIX_ROWS, D3D_SVT_FLOAT,       4, 4 },
		{ "sampler2D", D3D_SVC_OBJECT,         D3D_SVT_SAMPLER,     0, 0 },
		{ "sampler2D", D3D_SVC_OBJECT,         D3D_SVT_SAMPLER2D,   0, 0 },
		{ "sampler2D", D3D_SVC_OBJECT,         D3D_SVT_SAMPLER3D,   0, 0 },
		{ "samplerCube", D3D_SVC_OBJECT,         D3D_SVT_SAMPLERCUBE, 0, 0 },
	};

}

extern bool ComplieHLSLShader(ComplierState& gloablState);
