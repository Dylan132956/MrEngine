#pragma once
#include "CommandBuffer.h"

namespace moonriver
{
	typedef cb::CommandBuffer<cb::DrawKey, cb::DefaultKeyDecoder> GeometryCommandBuffer;
	typedef cb::CommandBuffer<uint32_t, cb::DummyKeyDecoder<uint32_t>> DeferredCommandBuffer;
	typedef cb::CommandBuffer<uint16_t, cb::DummyKeyDecoder<uint16_t>> PostProcessCommandBuffer;

	class CmdBuf
	{
	public:
		CmdBuf();
		~CmdBuf();

		GeometryCommandBuffer m_geometryCommands;
		DeferredCommandBuffer m_deferredCommands;
		PostProcessCommandBuffer m_postProcessCommands;

		static CmdBuf* CmdBufManager();
		static CmdBuf cb;
	};
}


