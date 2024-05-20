#include "Camera.h"
#include "Entity.h"
#include "Texture.h"
#include "RenderTarget.h"
#include "Engine.h"
#include "Renderer.h"
#include "Material.h"
#include "SkinnedMeshRenderer.h"
#include "Light.h"
#include "time/Time.h"
#include "Transform.h"
#include "postprocessing/PostProcessing.h"

namespace moonriver
{
	std::list<Camera*> Camera::m_cameras;
    WeakRef<Camera> Camera::m_main_camera;
	Camera* Camera::m_current_camera = nullptr;
	bool Camera::m_cameras_order_dirty = false;
	Ref<Mesh> Camera::m_quad_mesh;
	Ref<Material> Camera::m_blit_material;

	void Camera::Init()
	{
	
	}

	void Camera::Done()
	{
		m_quad_mesh.reset();
		m_blit_material.reset();
	}

	void Camera::RenderAll()
	{
		const auto& lights = Light::GetLights();
		for (auto i : lights)
		{
            if (i->GetEntity()->IsActiveInTree() && i->IsEnable())
            {
                i->Prepare();
            }
		}

		if (m_cameras_order_dirty)
		{
			m_cameras_order_dirty = false;
			m_cameras.sort([](Camera* a, Camera* b) {
				return a->GetDepth() < b->GetDepth();
			});
		}

		for (auto i : m_cameras)
		{
			if (i->GetEntity()->IsActiveInTree() && i->IsEnable())
			{
				m_current_camera = i;

				std::list<Renderer*> renderers;
				i->CullRenderers(Renderer::GetRenderers(), renderers);
				i->UpdateViewUniforms();
				i->Draw(renderers);
				i->DoPostProcessing();

				m_current_camera = nullptr;
			}
		}
	}
    
    void Camera::OnResizeAll(int width, int height)
    {
        for (auto i : m_cameras)
        {
            i->OnResize(width, height);
        }

		auto& renderers = Renderer::GetRenderers();
		for (auto i : renderers)
		{
			i->OnResize(width, height);
		}
    }
    
    void Camera::OnResize(int width, int height)
    {
        m_projection_matrix_dirty = true;
    }

    void Camera::CullRenderers(const std::list<Renderer*>& renderers, std::list<Renderer*>& result)
    {
        for (auto i : renderers)
        {
            int layer = i->GetEntity()->GetLayer();
            if (i->GetEntity()->IsActiveInTree() && i->IsEnable() && ((1 << layer) & m_culling_mask) != 0)
            {
                result.push_back(i);
            }
        }
        result.sort([](Renderer* a, Renderer* b) {
            const auto& materials_a = a->GetMaterials();
            int queue_a = 0;
            for (int i = 0; i < materials_a.size(); ++i)
            {
				if (materials_a[i])
				{
					int queue = materials_a[i]->GetQueue();
					if (queue_a < queue)
					{
						queue_a = queue;
					}
				}
            }
            const auto& materials_b = b->GetMaterials();
            int queue_b = 0;
            for (int i = 0; i < materials_b.size(); ++i)
            {
				if (materials_b[i])
				{
					int queue = materials_b[i]->GetQueue();
					if (queue_b < queue)
					{
						queue_b = queue;
					}
				}
            }
            return queue_a < queue_b;
        });
    }

	void Camera::UpdateViewUniforms()
	{
		auto& driver = Engine::Instance()->GetDriverApi();
		if (!m_view_uniform_buffer)
		{
			m_view_uniform_buffer = driver.createUniformBuffer(sizeof(ViewUniforms), filament::backend::BufferUsage::DYNAMIC);
		}

		m_view_uniforms.view_matrix = this->GetViewMatrix();
		m_view_uniforms.projection_matrix = this->GetProjectionMatrix();
		m_view_uniforms.camera_pos = this->GetTransform()->GetPosition();
		float t = Time::GetTime();
		m_view_uniforms.time = Vector4(t / 20, t, t * 2, t * 3);

		// map depth range -1 ~ 1 to 0 ~ 1 for d3d
		//if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
		//{
		//	m_view_uniforms.projection_matrix = Matrix4x4::ProjectionDepthMapD3D11() * this->GetProjectionMatrix();
		//}

		void* buffer = driver.allocate(sizeof(ViewUniforms));
		Memory::Copy(buffer, &m_view_uniforms, sizeof(ViewUniforms));
		driver.loadUniformBuffer(m_view_uniform_buffer, filament::backend::BufferDescriptor(buffer, sizeof(ViewUniforms)));
	}

	void Camera::Draw(const std::list<Renderer*>& renderers)
	{
		auto& driver = Engine::Instance()->GetDriverApi();

		int target_width = this->GetTargetWidth();
		int target_height = this->GetTargetHeight();
		bool has_post_processing = this->HasPostProcessing();

		filament::backend::RenderTargetHandle target;
		filament::backend::RenderPassParams params;
		params.flags.clear = filament::backend::TargetBufferFlags::NONE;
		params.flags.discardStart = filament::backend::TargetBufferFlags::NONE;
		params.flags.discardEnd = filament::backend::TargetBufferFlags::NONE;

		if (m_render_target_color || m_render_target_depth)
		{
			if (!m_render_target)
			{
				filament::backend::TargetBufferFlags target_flags = filament::backend::TargetBufferFlags::NONE;
				filament::backend::TargetBufferInfo color = { };
				filament::backend::TargetBufferInfo depth = { };
				filament::backend::TargetBufferInfo stencil = { };

				if (m_render_target_color)
				{
					target_flags |= filament::backend::TargetBufferFlags::COLOR;
					color.handle = m_render_target_color->GetTexture();
				}
				if (m_render_target_depth)
				{
					target_flags |= filament::backend::TargetBufferFlags::DEPTH;
					depth.handle = m_render_target_depth->GetTexture();
				}

				m_render_target = driver.createRenderTarget(
					target_flags,
					target_width,
					target_height,
					1,
					color,
					depth,
					stencil);
			}

			if (has_post_processing)
			{
				assert(m_render_target_color);

				filament::backend::TargetBufferFlags target_flags = filament::backend::TargetBufferFlags::NONE;
				TextureFormat color_format = TextureFormat::None;
				TextureFormat depth_format = TextureFormat::None;

				if (m_render_target_color)
				{
					target_flags |= filament::backend::TargetBufferFlags::COLOR;
					color_format = m_render_target_color->GetFormat();
				}
				if (m_render_target_depth)
				{
					target_flags |= filament::backend::TargetBufferFlags::DEPTH;
					depth_format = m_render_target_depth->GetFormat();
				}

				m_post_processing_target = RenderTarget::GetTemporaryRenderTarget(
					target_width,
					target_height,
					color_format,
					depth_format,
					FilterMode::Linear,
					SamplerAddressMode::ClampToEdge,
					target_flags);
				target = m_post_processing_target->target;
			}
			else
			{
				target = m_render_target;
			}

			switch (m_clear_flags)
			{
			case CameraClearFlags::Invalidate:
				params.flags.clear = filament::backend::TargetBufferFlags::NONE;
				if (m_render_target_color)
				{
					params.flags.discardStart |= filament::backend::TargetBufferFlags::COLOR;
				}
				if (m_render_target_depth)
				{
					params.flags.discardStart |= filament::backend::TargetBufferFlags::DEPTH;
				}
				break;
			case CameraClearFlags::Color:
				if (m_render_target_color)
				{
					params.flags.clear = filament::backend::TargetBufferFlags::COLOR;
				}
				break;
			case CameraClearFlags::Depth:
				if (m_render_target_depth)
				{
					params.flags.clear = filament::backend::TargetBufferFlags::DEPTH;
				}
				break;
			case CameraClearFlags::ColorAndDepth:
				params.flags.clear = filament::backend::TargetBufferFlags::NONE;
				if (m_render_target_color)
				{
					params.flags.clear |= filament::backend::TargetBufferFlags::COLOR;
				}
				if (m_render_target_depth)
				{
					params.flags.clear |= filament::backend::TargetBufferFlags::DEPTH;
				}
				break;
			case CameraClearFlags::Nothing:
				params.flags.clear = filament::backend::TargetBufferFlags::NONE;
				break;
			}
		}
		else
		{
			if (has_post_processing)
			{
				m_post_processing_target = RenderTarget::GetTemporaryRenderTarget(
					target_width,
					target_height,
					TextureFormat::R8G8B8A8,
					Texture::SelectDepthFormat(),
					FilterMode::Linear,
					SamplerAddressMode::ClampToEdge,
					filament::backend::TargetBufferFlags::COLOR_AND_DEPTH);
				target = m_post_processing_target->target;
			}
			else
			{
				target = *(filament::backend::RenderTargetHandle*) Engine::Instance()->GetDefaultRenderTarget();
			}

			switch (m_clear_flags)
			{
			case CameraClearFlags::Invalidate:
				params.flags.clear = filament::backend::TargetBufferFlags::NONE;
				params.flags.discardStart = filament::backend::TargetBufferFlags::COLOR_AND_DEPTH;
				break;
			case CameraClearFlags::Color:
				params.flags.clear = filament::backend::TargetBufferFlags::COLOR;
				break;
			case CameraClearFlags::Depth:
				params.flags.clear = filament::backend::TargetBufferFlags::DEPTH;
				break;
			case CameraClearFlags::ColorAndDepth:
				params.flags.clear = filament::backend::TargetBufferFlags::COLOR_AND_DEPTH;
				break;
			case CameraClearFlags::Nothing:
				params.flags.clear = filament::backend::TargetBufferFlags::NONE;
				break;
			}
		}

		params.viewport.left = (int32_t) (m_viewport_rect.x * target_width);
		params.viewport.bottom = (int32_t) ((1.0f - (m_viewport_rect.y + m_viewport_rect.h)) * target_height);
		params.viewport.width = (uint32_t) (m_viewport_rect.w * target_width);
		params.viewport.height = (uint32_t) (m_viewport_rect.h * target_height);
		params.clearColor = filament::math::float4(m_clear_color.r, m_clear_color.g, m_clear_color.b, m_clear_color.a);

		driver.beginRenderPass(target, params);

		driver.bindUniformBuffer((size_t) Shader::BindingPoint::PerView, m_view_uniform_buffer);

        for (auto i : renderers)
        {
            this->DrawRenderer(i);
        }
        
		driver.endRenderPass();

		driver.flush();
	}

    void Camera::DrawRenderer(Renderer* renderer)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
		
		driver.bindUniformBuffer((size_t) Shader::BindingPoint::PerRenderer, renderer->GetTransformUniformBuffer());

        SkinnedMeshRenderer* skin = dynamic_cast<SkinnedMeshRenderer*>(renderer);
        if (skin && skin->GetBonesUniformBuffer())
        {
            driver.bindUniformBuffer((size_t) Shader::BindingPoint::PerRendererBones, skin->GetBonesUniformBuffer());
        }
        if (skin && skin->GetBlendShapeSamplerGroup())
        {
            driver.bindSamplers((size_t) Shader::BindingPoint::PerRendererBones, skin->GetBlendShapeSamplerGroup());
        }

		bool lighted = false;
		bool light_add = false;
		const auto& lights = Light::GetLights();
		for (auto i : lights)
		{
			if ((1 << renderer->GetEntity()->GetLayer()) & i->GetCullingMask())
			{
				if (i->IsShadowEnable())
				{
					if (i->GetViewUniformBuffer())
					{
						driver.bindUniformBuffer((size_t) Shader::BindingPoint::PerLightVertex, i->GetViewUniformBuffer());
					}
					
					if (i->GetSamplerGroup())
					{
						driver.bindSamplers((size_t) Shader::BindingPoint::PerLightFragment, i->GetSamplerGroup());
					}
				}
				driver.bindUniformBuffer((size_t) Shader::BindingPoint::PerLightFragment, i->GetLightUniformBuffer());

                this->DoDraw(renderer, i->IsShadowEnable(), light_add);

				lighted = true;
				light_add = true;
			}
		}

		if (!lighted)
		{
            this->DoDraw(renderer);
		}

        //if (Engine::Instance()->GetEditor()->IsInEditorMode())
        //{
        //    if (renderer->GetLocalBounds().GetSize().SqrMagnitude() > 0)
        //    {
        //        this->DrawRendererBounds(renderer);
        //    }
        //}
    }

    void Camera::DoDraw(Renderer* renderer, bool shadow_enable, bool light_add)
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        SkinnedMeshRenderer* skin = dynamic_cast<SkinnedMeshRenderer*>(renderer);

        const auto& materials = renderer->GetMaterials();
        for (int i = 0; i < materials.size(); ++i)
        {
            auto& material = materials[i];
            if (material)
            {
                if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL &&
                    Engine::Instance()->GetShaderModel() == filament::backend::ShaderModel::GL_ES_20)
                {
                    material->SetMatrix(ViewUniforms::VIEW_MATRIX, m_view_uniforms.view_matrix);
                    material->SetMatrix(ViewUniforms::PROJECTION_MATRIX, m_view_uniforms.projection_matrix);
                    material->SetVector(ViewUniforms::CAMERA_POS, m_view_uniforms.camera_pos);
                    material->SetVector(ViewUniforms::TIME, m_view_uniforms.time);
                    material->SetMatrix(RendererUniforms::MODEL_MATRIX, renderer->GetTransform()->GetLocalToWorldMatrix());

                    if (skin && skin->GetBonesUniformBuffer())
                    {
                        material->SetVectorArray(SkinnedMeshRendererUniforms::BONES, skin->GetBoneVectors());
                    }
                }

                filament::backend::RenderPrimitiveHandle primitive;

                auto primitives = renderer->GetPrimitives();
                if (i < primitives.size())
                {
                    primitive = primitives[i];
                }
                else if (primitives.size() > 0)
                {
                    primitive = primitives[0];
                }

                if (primitive)
                {
                    auto keywords = renderer->GetShaderKeywords();
                    if (shadow_enable && renderer->IsRecieveShadow())
                    {
                        keywords.push_back("RECIEVE_SHADOW_ON");
                    }
                    if (light_add)
                    {
                        keywords.push_back("LIGHT_ADD_ON");
                    }
                    
                    const auto& shader = material->GetShader(keywords);

                    material->SetScissor(this->GetTargetWidth(), this->GetTargetHeight());

                    for (int j = 0; j < shader->GetPassCount(); ++j)
                    {
                        bool has_light = shader->GetPass(j).light_mode == Shader::LightMode::Forward;
                        if (!has_light && light_add)
                        {
                            continue;
                        }

                        material->Bind(shader, j);

                        const auto& pipeline = shader->GetPass(j).pipeline;
                        driver.draw(pipeline, primitive);
                        Time::SetDrawCall(Time::GetDrawCall() + 1);
                    }
                }
            }
        }
    }

    void Camera::DrawRendererBounds(Renderer* renderer)
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        auto& material = Material::GetSharedBoundsMaterial();
        auto primitive = Mesh::GetSharedBoundsMesh()->GetPrimitives()[0];

        if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL &&
            Engine::Instance()->GetShaderModel() == filament::backend::ShaderModel::GL_ES_20)
        {
            auto& renderer_uniforms = renderer->GetRendererUniforms();

            material->SetMatrix(ViewUniforms::VIEW_MATRIX, m_view_uniforms.view_matrix);
            material->SetMatrix(ViewUniforms::PROJECTION_MATRIX, m_view_uniforms.projection_matrix);
            material->SetMatrix(RendererUniforms::MODEL_MATRIX, renderer_uniforms.model_matrix);
            material->SetMatrix(RendererUniforms::BOUNDS_MATRIX, renderer_uniforms.bounds_matrix);
            material->SetColor(RendererUniforms::BOUNDS_COLOR, renderer_uniforms.bounds_color);
        }

        if (primitive)
        {
            const auto& shader = material->GetShader();

            material->SetScissor(this->GetTargetWidth(), this->GetTargetHeight());

            for (int j = 0; j < shader->GetPassCount(); ++j)
            {
                material->Bind(shader, j);

                const auto& pipeline = shader->GetPass(j).pipeline;
                driver.draw(pipeline, primitive);
            }
        }
    }

	bool Camera::HasPostProcessing()
	{
        std::vector<std::shared_ptr<PostProcessing>> coms = this->GetEntity()->GetComponents<PostProcessing>();
        return coms.size() > 0;
	}

	void Camera::DoPostProcessing()
	{
		std::vector<std::shared_ptr<PostProcessing>> coms = this->GetEntity()->GetComponents<PostProcessing>();
		if (coms.size() == 0)
		{
			return;
		}

		int target_width = this->GetTargetWidth();
		int target_height = this->GetTargetHeight();

		Ref<RenderTarget> post_processing_target_1;
		Ref<RenderTarget> post_processing_target_2;
		if (coms.size() > 1)
		{
			post_processing_target_1 = RenderTarget::GetTemporaryRenderTarget(
				target_width,
				target_height,
				TextureFormat::R8G8B8A8,
				TextureFormat::None,
				FilterMode::Linear,
				SamplerAddressMode::ClampToEdge,
				filament::backend::TargetBufferFlags::COLOR);
		}
		if (coms.size() > 2)
		{
			post_processing_target_2 = RenderTarget::GetTemporaryRenderTarget(
				target_width,
				target_height,
				TextureFormat::R8G8B8A8,
				TextureFormat::None,
				FilterMode::Linear,
				SamplerAddressMode::ClampToEdge,
				filament::backend::TargetBufferFlags::COLOR);
		}

		Ref<RenderTarget> src = m_post_processing_target;
		Ref<RenderTarget> dst = post_processing_target_1;

		for (int i = 0; i < coms.size(); ++i)
		{
			if (i == coms.size() - 1)
			{
				dst = RefMake<RenderTarget>();
				dst->key.width = target_width;
				dst->key.height = target_height;
				dst->key.filter_mode = FilterMode::Nearest;
				dst->key.wrap_mode = SamplerAddressMode::ClampToEdge;

				if (m_render_target_color || m_render_target_depth)
				{
					filament::backend::TargetBufferFlags target_flags = filament::backend::TargetBufferFlags::NONE;
					TextureFormat color_format = TextureFormat::None;
					TextureFormat depth_format = TextureFormat::None;

					if (m_render_target_color)
					{
						target_flags |= filament::backend::TargetBufferFlags::COLOR;
						color_format = m_render_target_color->GetFormat();
					}
					if (m_render_target_depth)
					{
						target_flags |= filament::backend::TargetBufferFlags::DEPTH;
						depth_format = m_render_target_depth->GetFormat();
					}

					dst->key.color_format = color_format;
					dst->key.depth_format = depth_format;
					dst->key.flags = target_flags;

					dst->target = m_render_target;
				}
				else
				{
					dst->key.color_format = TextureFormat::R8G8B8A8;
					dst->key.depth_format = Texture::SelectDepthFormat();
					dst->key.flags = filament::backend::TargetBufferFlags::COLOR_AND_DEPTH;

					dst->target = *(filament::backend::RenderTargetHandle*) Engine::Instance()->GetDefaultRenderTarget();
				}
			}

			coms[i]->SetCameraDepthTexture(m_post_processing_target->depth);
			coms[i]->OnRenderImage(src, dst);
			coms[i]->SetCameraDepthTexture(Ref<Texture>());

			// swap
			if (i < coms.size() - 1)
			{
				Ref<RenderTarget> temp = src;
				src = dst;
				dst = temp;

				if (i == 0 && coms.size() > 2)
				{
					dst = post_processing_target_2;
				}
			}
		}

		if (post_processing_target_1)
		{
			RenderTarget::ReleaseTemporaryRenderTarget(post_processing_target_1);
			post_processing_target_1.reset();
		}
		if (post_processing_target_2)
		{
			RenderTarget::ReleaseTemporaryRenderTarget(post_processing_target_2);
			post_processing_target_2.reset();
		}

		RenderTarget::ReleaseTemporaryRenderTarget(m_post_processing_target);
		m_post_processing_target.reset();
	}

	void Camera::Blit(const Ref<RenderTarget>& src, const Ref<RenderTarget>& dst, const Ref<Material>& mat, int pass)
	{
		int target_width = dst->key.width;
		int target_height = dst->key.height;

		filament::backend::RenderPassParams params;
		params.flags.clear = filament::backend::TargetBufferFlags::COLOR;
		if (dst->target == m_current_camera->m_render_target ||
			dst->target == *(filament::backend::RenderTargetHandle*) Engine::Instance()->GetDefaultRenderTarget())
		{
			params.flags.clear = dst->key.flags;
		}

		params.viewport.left = 0;
		params.viewport.bottom = 0;
		params.viewport.width = (uint32_t) target_width;
		params.viewport.height = (uint32_t) target_height;
		params.clearColor = filament::math::float4(0, 0, 0, 0);

		// draw quad
		filament::backend::RenderPrimitiveHandle primitive;
		if (!m_quad_mesh)
		{
			std::vector<Mesh::Vertex> vertices(4);
			vertices[0].vertex = Vector3(-1, 1, 0);
			vertices[1].vertex = Vector3(-1, -1, 0);
			vertices[2].vertex = Vector3(1, -1, 0);
			vertices[3].vertex = Vector3(1, 1, 0);
			
			if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL)
			{
				vertices[0].uv = Vector2(0, 1);
				vertices[1].uv = Vector2(0, 0);
				vertices[2].uv = Vector2(1, 0);
				vertices[3].uv = Vector2(1, 1);
			}
			else
			{
				vertices[0].uv = Vector2(0, 0);
				vertices[1].uv = Vector2(0, 1);
				vertices[2].uv = Vector2(1, 1);
				vertices[3].uv = Vector2(1, 0);
			}

			std::vector<unsigned int> indices = {
				0, 1, 2, 0, 2, 3
			};

			m_quad_mesh = std::make_shared<Mesh>(std::move(vertices), std::move(indices));
		}
		primitive = m_quad_mesh->GetPrimitives()[0];

		Ref<Material> material = mat;
		if (!material)
		{
			if (!m_blit_material)
			{
				m_blit_material = std::make_shared<Material>(Shader::Find("Blit"));
			}
			material = m_blit_material;

			material->SetTexture(MaterialProperty::TEXTURE, src->color);
		}

		if (primitive && material)
		{
			material->Prepare(pass);

			auto& driver = Engine::Instance()->GetDriverApi();
			driver.beginRenderPass(dst->target, params);

			const auto& shader = material->GetShader();
			material->SetScissor(target_width, target_height);

			int pass_begin = 0;
			int pass_end = shader->GetPassCount();
			if (pass >= 0 && pass < shader->GetPassCount())
			{
				pass_begin = pass;
				pass_end = pass + 1;
			}

			for (int i = pass_begin; i < pass_end; ++i)
			{
				material->Bind(shader, i);

				const auto& pipeline = shader->GetPass(i).pipeline;
				driver.draw(pipeline, primitive);
				Time::SetDrawCall(Time::GetDrawCall() + 1);
			}

			driver.endRenderPass();
			driver.flush();
		}
	}

	Camera::Camera():
		m_depth(0),
        m_culling_mask(0xffffffff),
		m_clear_flags(CameraClearFlags::ColorAndDepth),
		m_clear_color(0, 0, 0, 1),
		m_viewport_rect(0, 0, 1, 1),
		m_field_of_view(45),
        m_aspect(-1),
		m_near_clip(0.3f),
		m_far_clip(1000),
		m_orthographic(false),
		m_orthographic_size(1),
		m_view_matrix_dirty(true),
		m_projection_matrix_dirty(true),
		m_view_matrix_external(false),
		m_projection_matrix_external(false)
    {
		m_cameras.push_back(this);
		m_cameras_order_dirty = true;
    }
    
	Camera::~Camera()
    {
		auto& driver = Engine::Instance()->GetDriverApi();

		if (m_view_uniform_buffer)
		{
			driver.destroyUniformBuffer(m_view_uniform_buffer);
			m_view_uniform_buffer.clear();
		}

		if (m_render_target)
		{
			driver.destroyRenderTarget(m_render_target);
			m_render_target.clear();
		}

        for (auto i = m_cameras.begin(); i != m_cameras.end(); ++i)
        {
            if (this == *i)
            {
                m_cameras.erase(i);
                break;
            }
        }
    }

	void Camera::OnTransformDirty()
	{
		m_view_matrix_dirty = true;
	}

	void Camera::SetDepth(int depth)
	{
		m_depth = depth;
	}

    void Camera::SetCullingMask(uint32_t mask)
    {
        m_culling_mask = mask;
    }
    
	void Camera::SetClearFlags(CameraClearFlags flags)
	{
		m_clear_flags = flags;
	}

	void Camera::SetClearColor(const Color& color)
	{
		m_clear_color = color;
	}

	void Camera::SetViewportRect(const Rect& rect)
	{
        if (m_viewport_rect != rect)
        {
            m_viewport_rect = rect;
            m_projection_matrix_dirty = true;
        }
	}

	void Camera::SetFieldOfView(float fov)
	{
        if (!Mathf::FloatEqual(m_field_of_view, fov))
        {
            m_field_of_view = fov;
            m_projection_matrix_dirty = true;
        }
	}
    
    float Camera::GetAspect() const
    {
        if (m_aspect > 0)
        {
            return m_aspect;
        }
        else
        {
            return this->GetTargetWidth() / (float) this->GetTargetHeight();
        }
    }
    
    void Camera::SetAspect(float aspect)
    {
        if (!Mathf::FloatEqual(m_aspect, aspect))
        {
            m_aspect = aspect;
            m_projection_matrix_dirty = true;
        }
    }

	void Camera::SetNearClip(float clip)
	{
        if (!Mathf::FloatEqual(m_near_clip, clip))
        {
            m_near_clip = clip;
            m_projection_matrix_dirty = true;
        }
	}

	void Camera::SetFarClip(float clip)
	{
        if (!Mathf::FloatEqual(m_far_clip, clip))
        {
            m_far_clip = clip;
            m_projection_matrix_dirty = true;
        }
	}

	void Camera::SetOrthographic(bool enable)
	{
        if (m_orthographic != enable)
        {
            m_orthographic = enable;
            m_projection_matrix_dirty = true;
        }
	}

	void Camera::SetOrthographicSize(float size)
	{
        if (!Mathf::FloatEqual(m_orthographic_size, size))
        {
            m_orthographic_size = size;
            m_projection_matrix_dirty = true;
        }
	}

	const Matrix4x4& Camera::GetViewMatrix()
	{
		if (m_view_matrix_dirty)
		{
			m_view_matrix_dirty = false;

			if (!m_view_matrix_external)
			{
                if (m_leftHandSpace)
                {
                    //+z
                    Vector3 target = this->GetTransform()->GetPosition() + this->GetTransform()->GetForward();
                    m_view_matrix = Matrix4x4::LookAtLH(this->GetTransform()->GetPosition(), target, this->GetTransform()->GetUp());
                }
                else
                {
                    //-z
                    Vector3 target = this->GetTransform()->GetPosition() + this->GetTransform()->GetForward();
                    m_view_matrix = Matrix4x4::LookAtRH(this->GetTransform()->GetPosition(), target, this->GetTransform()->GetUp());
                }
			}
		}

		return m_view_matrix;
	}

    void Camera::SetLeftHandSpace(bool enable)
    {
        m_leftHandSpace = enable;
        m_view_matrix_dirty = true;
    }

	const Matrix4x4& Camera::GetProjectionMatrix()
	{
		if (m_projection_matrix_dirty)
		{
			m_projection_matrix_dirty = false;

			if (!m_projection_matrix_external)
			{
				float view_width = this->GetTargetWidth() * m_viewport_rect.w;
				float view_height = this->GetTargetHeight() * m_viewport_rect.h;
                float aspect = m_aspect;
                if (aspect <= 0)
                {
                    aspect = view_width / view_height;
                }

				if (m_orthographic)
				{
					float ortho_size = m_orthographic_size;
					float top = ortho_size;
					float bottom = -ortho_size;
					float plane_h = ortho_size * 2;
					float plane_w = plane_h * aspect;
					m_projection_matrix = Matrix4x4::Ortho(-plane_w / 2, plane_w / 2, bottom, top, m_near_clip, m_far_clip);
				}
				else
				{
					m_projection_matrix = Matrix4x4::Perspective(m_field_of_view, aspect, m_near_clip, m_far_clip);
				}
			}
		}

		return m_projection_matrix;
	}

	void Camera::SetViewMatrixExternal(const Matrix4x4& mat)
	{
		m_view_matrix = mat;
		m_view_matrix_dirty = true;
		m_view_matrix_external = true;
	}

	void Camera::SetProjectionMatrixExternal(const Matrix4x4& mat)
	{
		m_projection_matrix = mat;
		m_projection_matrix_dirty = true;
		m_projection_matrix_external = true;
	}

	void Camera::SetRenderTarget(const Ref<Texture>& color, const Ref<Texture>& depth)
	{
		m_render_target_color = color;
		m_render_target_depth = depth;
		m_projection_matrix_dirty = true;

		auto& driver = Engine::Instance()->GetDriverApi();
		if (m_render_target)
		{
			driver.destroyRenderTarget(m_render_target);
			m_render_target.clear();
		}
	}

	int Camera::GetTargetWidth() const
	{
		if (m_render_target_color)
		{
			return m_render_target_color->GetWidth();
		}
		else if (m_render_target_depth)
		{
			return m_render_target_depth->GetWidth();
		}
		else
		{
			return Engine::Instance()->GetWidth();
		}
	}

	int Camera::GetTargetHeight() const
	{
		if (m_render_target_color)
		{
			return m_render_target_color->GetHeight();
		}
		else if (m_render_target_depth)
		{
			return m_render_target_depth->GetHeight();
		}
		else
		{
			return Engine::Instance()->GetHeight();
		}
	}

    Vector3 Camera::ScreenToViewportPoint(const Vector3& position)
    {
        float x = (position.x / this->GetTargetWidth() - m_viewport_rect.x) / m_viewport_rect.w;
        float y = (position.y / this->GetTargetHeight() - m_viewport_rect.y) / m_viewport_rect.h;
        return Vector3(x, y, position.z);
    }

    Vector3 Camera::ViewportToScreenPoint(const Vector3& position)
    {
        float x = (position.x * m_viewport_rect.w + m_viewport_rect.x) * this->GetTargetWidth();
        float y = (position.y * m_viewport_rect.h + m_viewport_rect.y) * this->GetTargetHeight();
        return Vector3(x, y, position.z);
    }

    Vector3 Camera::ScreenToWorldPoint(const Vector3& position)
    {
        Vector3 pos_viewport = this->ScreenToViewportPoint(position);
        Vector3 pos_proj = pos_viewport * 2.0f - Vector3(1.0f, 1.0f, 0);
        Matrix4x4 vp_inverse = (this->GetProjectionMatrix() * this->GetViewMatrix()).Inverse();

        if (this->IsOrthographic())
        {
            Vector4 pos_world = vp_inverse * Vector4(pos_proj.x, pos_proj.y, 0, 1.0f);
            pos_world *= 1.0f / pos_world.w;

            Vector3 origin = Vector3(pos_world.x, pos_world.y, pos_world.z);
            Vector3 direction = this->GetTransform()->GetForward();

            Ray ray_screen(origin, direction);
            float ray_len = position.z - this->GetNearClip();

            return ray_screen.GetPoint(ray_len);
        }
        else
        {
            Vector4 pos_world = vp_inverse * Vector4(pos_proj.x, pos_proj.y, -1.0f, 1.0f);
            pos_world *= 1.0f / pos_world.w;

            Vector3 origin = this->GetTransform()->GetPosition();
            Vector3 direction = Vector3(pos_world.x, pos_world.y, pos_world.z) - origin;

            Ray ray_screen(origin, direction);
            Ray ray_forward(origin, this->GetTransform()->GetForward());
            Vector3 plane_point = ray_forward.GetPoint(position.z);
            float ray_len;
            bool hit = Mathf::RayPlaneIntersection(ray_screen, ray_forward.GetDirection(), plane_point, ray_len);
            assert(hit);

            return ray_screen.GetPoint(ray_len);
        }
    }

    Vector3 Camera::WorldToScreenPoint(const Vector3& position)
    {
        Vector3 pos_view = this->GetViewMatrix().MultiplyPoint3x4(position);
        Vector3 pos_proj = this->GetProjectionMatrix().MultiplyPoint3x4(pos_view);
        Vector3 pos_viewport = (pos_proj + Vector3(1.0f, 1.0f, 0)) * 0.5f;
        pos_viewport.z = pos_view.z;
        return this->ViewportToScreenPoint(pos_viewport);
    }

    Ray Camera::ScreenPointToRay(const Vector3& position)
    {
        Vector3 pos_world = this->ScreenToWorldPoint(Vector3(position.x, position.y, this->GetNearClip()));

        Vector3 origin = pos_world;
        Vector3 direction;

        if (this->IsOrthographic())
        {
            direction = this->GetTransform()->GetForward();
        }
        else
        {
            direction = origin - this->GetTransform()->GetPosition();
        }

        return Ray(origin, direction);
    }
}
