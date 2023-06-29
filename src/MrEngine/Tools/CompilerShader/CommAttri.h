#pragma once

#include <stdio.h>
#include <string>
#include <vector>

namespace commattri
{
	struct Attrib
	{
		/// Corresponds to vertex shader attribute.
		enum Enum
		{
			Position,  //!< a_position
			Normal,    //!< a_normal
			Tangent,   //!< a_tangent
			Bitangent, //!< a_bitangent
			Color0,    //!< a_color0
			Color1,    //!< a_color1
			Color2,    //!< a_color2
			Color3,    //!< a_color3
			Indices,   //!< a_indices
			Weight,    //!< a_weight
			TexCoord0, //!< a_texcoord0
			TexCoord1, //!< a_texcoord1
			TexCoord2, //!< a_texcoord2
			TexCoord3, //!< a_texcoord3
			TexCoord4, //!< a_texcoord4
			TexCoord5, //!< a_texcoord5
			TexCoord6, //!< a_texcoord6
			TexCoord7, //!< a_texcoord7

			Count
		};
	};

	struct UniformType
	{
		/// Uniform types:
		enum Enum
		{
			Sampler, //!< Sampler.
			End,     //!< Reserved, do not use.

			Float,   //!< float
			Vec2,    //!< 2 floats vector.
			Vec3,    //!< 3 floats vector.
			Vec4,    //!< 4 floats vector.
			Mat3,    //!< 3x3 matrix.
			Mat4,    //!< 4x4 matrix.

			Count
		};
	};

	static const std::string UniformTypeString[] =
	{
		"Sampler",   //!< Sampler.
		"End",       //!< Reserved, do not use.

		"float",     //!< float
		"float2",    //!< 2 floats vector.
		"float3",    //!< 3 floats vector.
		"float4",    //!< 4 floats vector.
		"float3x3",  //!< 3x3 matrix.
		"float4x4",  //!< 4x4 matrix.

		"Count"
	};

	struct AttribToId
	{
		Attrib::Enum attr;
		uint16_t id;
	};

	typedef std::pair<std::string, std::string> AttributeSlot;

	static AttribToId s_attribToId[] =
	{
		// NOTICE:
		// Attrib must be in order how it appears in Attrib::Enum! id is
		// unique and should not be changed if new Attribs are added.
		{ Attrib::Position,  0x0001 },
		{ Attrib::Normal,    0x0002 },
		{ Attrib::Tangent,   0x0003 },
		{ Attrib::Bitangent, 0x0004 },
		{ Attrib::Color0,    0x0005 },
		{ Attrib::Color1,    0x0006 },
		{ Attrib::Color2,    0x0018 },
		{ Attrib::Color3,    0x0019 },
		{ Attrib::Indices,   0x000e },
		{ Attrib::Weight,    0x000f },
		{ Attrib::TexCoord0, 0x0010 },
		{ Attrib::TexCoord1, 0x0011 },
		{ Attrib::TexCoord2, 0x0012 },
		{ Attrib::TexCoord3, 0x0013 },
		{ Attrib::TexCoord4, 0x0014 },
		{ Attrib::TexCoord5, 0x0015 },
		{ Attrib::TexCoord6, 0x0016 },
		{ Attrib::TexCoord7, 0x0017 },
	};

	struct RemapInputSemantic
	{
		commattri::Attrib::Enum m_attr;
		const char* m_name;
		const char* m_aliasName;
		uint8_t m_index;
	};

	static const RemapInputSemantic s_remapInputSemantic[Attrib::Count + 1] =
	{
		{ Attrib::Position,  "POSITION",  "",   0 },
		{ Attrib::Normal,    "NORMAL",    "",   0 },
		{ Attrib::Tangent,   "TANGENT",   "",   0 },
		{ Attrib::Bitangent, "BITANGENT",  "BINORMAL",  0 },
		{ Attrib::Color0,    "COLOR",     "",   0 },
		{ Attrib::Color1,    "COLOR",     "",   1 },
		{ Attrib::Color2,    "COLOR",     "",   2 },
		{ Attrib::Color3,    "COLOR",     "",   3 },
		{ Attrib::Indices,   "BLENDINDICES", "", 0 },
		{ Attrib::Weight,    "BLENDWEIGHT",  "", 0 },
		{ Attrib::TexCoord0, "TEXCOORD",    "", 0 },
		{ Attrib::TexCoord1, "TEXCOORD",   "",  1 },
		{ Attrib::TexCoord2, "TEXCOORD",   "",  2 },
		{ Attrib::TexCoord3, "TEXCOORD",   "",  3 },
		{ Attrib::TexCoord4, "TEXCOORD",   "",  4 },
		{ Attrib::TexCoord5, "TEXCOORD",   "",  5 },
		{ Attrib::TexCoord6, "TEXCOORD",   "",  6 },
		{ Attrib::TexCoord7, "TEXCOORD",   "",  7 },
		{ Attrib::Count,     "",           "",  0 },
	};

	static uint16_t attribToId(Attrib::Enum _attr)
	{
		return s_attribToId[_attr].id;
	}

	static std::string GetAttributeTypeStr(Attrib::Enum _type)
	{
		for (uint32_t ii = 0; ii < Attrib::Count; ++ii)
		{
			const RemapInputSemantic& ris = s_remapInputSemantic[ii];
			if (ris.m_attr == _type)
			{
				return ris.m_name;
			}
		}

		return s_remapInputSemantic[Attrib::Count].m_name;
	}


	static const commattri::RemapInputSemantic& findInputSemantic(const char* _name, uint8_t _index)
	{
		for (uint32_t ii = 0; ii < commattri::Attrib::Count; ++ii)
		{
			const commattri::RemapInputSemantic& ris = commattri::s_remapInputSemantic[ii];
			bool nameEqual = (0 == std::strcmp(ris.m_name, _name) || 0 == std::strcmp(ris.m_aliasName, _name));

			if (nameEqual && ris.m_index == _index)
			{
				return ris;
			}
		}

		return commattri::s_remapInputSemantic[commattri::Attrib::Count];
	}
}

