#include "RenderTarget.h"
#include "Engine.h"
#include <algorithm>

namespace moonriver
{
	std::map<uint64_t, TemporaryRenderTargets> RenderTarget::m_temporary_render_targets_using;
	std::map<uint64_t, TemporaryRenderTargets> RenderTarget::m_temporary_render_targets_idle;

	void RenderTarget::Init()
	{

	}

	void RenderTarget::Done()
	{
		auto& driver = Engine::Instance()->GetDriverApi();

		for (const auto& i : m_temporary_render_targets_using)
		{
			const auto& targets = i.second.targets;
			assert(targets.size() == 0);
		}
		m_temporary_render_targets_using.clear();

		for (const auto& i : m_temporary_render_targets_idle)
		{
			const auto& targets = i.second.targets;
			for (int j = 0; j < targets.size(); ++j)
			{
				auto& target = targets[j]->target;
				driver.destroyRenderTarget(target);
				target.clear();
			}
		}
		m_temporary_render_targets_idle.clear();
	}

    std::shared_ptr <RenderTarget> RenderTarget::GetTemporaryRenderTarget(
		int width,
		int height,
		TextureFormat color_format,
		TextureFormat depth_format,
		FilterMode filter_mode,
		SamplerAddressMode wrap_mode,
		filament::backend::TargetBufferFlags flags)
	{
        std::shared_ptr <RenderTarget> target;

		RenderTargetKey key;
		key.width = width;
		key.height = height;
		key.color_format = color_format;
		key.depth_format = depth_format;
		key.filter_mode = filter_mode;
		key.wrap_mode = wrap_mode;
		key.flags = flags;

		TemporaryRenderTargets* p;
		bool create = false;
        std::map<uint64_t, TemporaryRenderTargets>::iterator it = m_temporary_render_targets_idle.find(key.u);
		if (it != m_temporary_render_targets_idle.end())
		{
            p = &it->second;
			if (p->targets.size() > 0)
			{
				int index = p->targets.size() - 1;
				target = p->targets[index];
				p->targets.erase(p->targets.begin() + index);
			}
			else
			{
				create = true;
			}
		}
		else
		{
			create = true;
		}

		if (create)
		{
			target = RefMake<RenderTarget>();
			target->key = key;

			filament::backend::TargetBufferInfo color = { };
			filament::backend::TargetBufferInfo depth = { };
			filament::backend::TargetBufferInfo stencil = { };

			if (flags & filament::backend::TargetBufferFlags::COLOR)
			{
				target->color = Texture::CreateRenderTexture(
					width,
					height,
					color_format,
					filter_mode,
					wrap_mode);
				color.handle = target->color->GetTexture();
			}

			if (flags & filament::backend::TargetBufferFlags::DEPTH)
			{
				target->depth = Texture::CreateRenderTexture(
					width,
					height,
					depth_format,
					filter_mode,
					wrap_mode);

				depth.handle = target->depth->GetTexture();
			}

			auto& driver = Engine::Instance()->GetDriverApi();
			target->target = driver.createRenderTarget(
				flags,
				width,
				height,
				1,
				color,
				depth,
				stencil);
		}

        std::map<uint64_t, TemporaryRenderTargets>::iterator itTemp = m_temporary_render_targets_using.find(key.u);
		if (itTemp != m_temporary_render_targets_using.end())
		{
            p = &itTemp->second;
			p->targets.push_back(target);
		}
		else
		{
			TemporaryRenderTargets targets;
			targets.key = key;
			targets.targets.push_back(target);

			m_temporary_render_targets_using[key.u] = targets;
		}

		return target;
	}

    void RenderTarget::ReleaseTemporaryRenderTarget(const std::shared_ptr<RenderTarget>& target)
	{
		RenderTargetKey key = target->key;

		TemporaryRenderTargets* p;
        std::map<uint64_t, TemporaryRenderTargets>::iterator it = m_temporary_render_targets_using.find(key.u);
		if (it != m_temporary_render_targets_using.end())
		{
            p = &it->second;
            //std::vector<std::shared_ptr<RenderTarget>>::iterator itRT = std::find(p->targets.begin(), p->targets.end(), target);
            for (size_t i = 0; i <= p->targets.size(); ++i)
            {
                if (p->targets[i] == target) {
                    std::map<uint64_t, TemporaryRenderTargets>::iterator itTemp = m_temporary_render_targets_idle.find(key.u);
                    if (itTemp != m_temporary_render_targets_idle.end())
                    {
                        p = &itTemp->second;
                        p->targets.push_back(target);
                    }
                    else
                    {
                        TemporaryRenderTargets targets;
                        targets.key = key;
                        targets.targets.push_back(target);

                        m_temporary_render_targets_idle[key.u] = targets;
                    }
                    p->targets.erase(p->targets.begin() + i);
                    break;
                }
            }
		}
	}
}
