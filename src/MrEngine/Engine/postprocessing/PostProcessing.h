#pragma once

#include "Component.h"

namespace moonriver
{
	class RenderTarget;
	class Texture;

	class PostProcessing : public Component
	{
	public:
		PostProcessing();
		virtual ~PostProcessing();
		virtual void OnRenderImage(const std::shared_ptr<RenderTarget>& src, const std::shared_ptr<RenderTarget>& dst);

	protected:
		friend class Camera;
		const void SetCameraDepthTexture(const std::shared_ptr<Texture>& texture) { m_camera_depth_texture = texture; }
		const std::shared_ptr<Texture>& GetCameraDepthTexture() const { return m_camera_depth_texture; }

	private:
        std::shared_ptr<Texture> m_camera_depth_texture;
	};
}
