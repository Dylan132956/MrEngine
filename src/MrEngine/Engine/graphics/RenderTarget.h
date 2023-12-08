
#pragma once

#include "Texture.h"
#include "private/backend/DriverApi.h"
#include <map>

namespace moonriver
{
	class RenderTargetKey
	{
	public:
		union
		{
			struct
			{
				int width									: 16;
				int height									: 16;
				TextureFormat color_format					: 8;
				TextureFormat depth_format					: 8;
				FilterMode filter_mode						: 4;
				SamplerAddressMode wrap_mode				: 4;
				filament::backend::TargetBufferFlags flags	: 4;
				int padding									: 4;
			};
			uint64_t u = 0;
		};
	};

	class RenderTarget;

	class TemporaryRenderTargets
	{
	public:
		RenderTargetKey key;
        std::vector<std::shared_ptr<RenderTarget>> targets;
	};

	class RenderTarget
	{
	public:
		static void Init();
		static void Done();
		static std::shared_ptr<RenderTarget> GetTemporaryRenderTarget(
			int width,
			int height,
			TextureFormat color_format,
			TextureFormat depth_format,
			FilterMode filter_mode,
			SamplerAddressMode wrap_mode,
			filament::backend::TargetBufferFlags flags);
		static void ReleaseTemporaryRenderTarget(const std::shared_ptr<RenderTarget>& target);

	public:
		filament::backend::RenderTargetHandle target;
        std::shared_ptr<Texture> color;
        std::shared_ptr<Texture> depth;
		RenderTargetKey key;

	private:
		static std::map<uint64_t, TemporaryRenderTargets> m_temporary_render_targets_using;
		static std::map<uint64_t, TemporaryRenderTargets> m_temporary_render_targets_idle;
	};
}
