#include "PostProcessing.h"
#include "graphics/Camera.h"

namespace moonriver
{
	PostProcessing::PostProcessing()
	{
	
	}

	PostProcessing::~PostProcessing()
	{
	
	}

    void PostProcessing::OnRenderImage(const std::shared_ptr<RenderTarget>& src, const std::shared_ptr<RenderTarget>& dst)
	{
		Camera::Blit(src, dst);
	}
}
