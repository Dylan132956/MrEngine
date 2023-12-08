#include "Renderer.h"
#include "Engine.h"
#include "Entity.h"
#include "Transform.h"
#include <algorithm>

namespace moonriver
{
    std::list<Renderer*> Renderer::m_renderers;

	void Renderer::PrepareAll()
	{
		for (auto i : m_renderers)
		{
            if (i->GetEntity()->IsActiveInTree() && i->IsEnable())
            {
                i->Prepare();
            }
		}
	}

    Renderer::Renderer():
		m_cast_shadow(false),
		m_recieve_shadow(false),
        m_lightmap_scale_offset(1, 1, 0, 0),
        m_lightmap_index(-1)
    {
        m_renderers.push_back(this);
    }
    
    Renderer::~Renderer()
    {
		auto& driver = Engine::Instance()->GetDriverApi();

		if (m_transform_uniform_buffer)
		{
			driver.destroyUniformBuffer(m_transform_uniform_buffer);
			m_transform_uniform_buffer.clear();
		}

        m_renderers.remove(this);
    }
    
    std::shared_ptr<Material> Renderer::GetMaterial() const
    {
        Ref<Material> material;
        
        if (m_materials.size() > 0)
        {
            material = m_materials[0];
        }
        
        return material;
    }
    
    void Renderer::SetMaterial(const std::shared_ptr<Material>& material)
    {
        this->SetMaterials({ material });
    }
    
    void Renderer::SetMaterials(const std::vector<std::shared_ptr<Material>>& materials)
    {
        m_materials = materials;

        this->UpdateShaderKeywords();
    }

	void Renderer::EnableCastShadow(bool enable)
	{
		m_cast_shadow = enable;
	}
    
	void Renderer::EnableRecieveShadow(bool enable)
	{
		m_recieve_shadow = enable;
	}

    void Renderer::SetLightmapIndex(int index)
    {
        m_lightmap_index = index;
    }
    
    void Renderer::SetLightmapScaleOffset(const Vector4& vec)
    {
        m_lightmap_scale_offset = vec;
    }

    void Renderer::SetShaderKeywords(const std::vector<std::string>& keywords)
    {
        m_shader_keywords = keywords;

        this->UpdateShaderKeywords();
    }

    void Renderer::EnableShaderKeyword(const std::string& keyword)
    {
        if (std::find(m_shader_keywords.begin(), m_shader_keywords.end(), keyword) == m_shader_keywords.end())
        {
            m_shader_keywords.push_back(keyword);

            this->UpdateShaderKeywords();
        }
    }

    const std::string& Renderer::GetShaderKey(int material_index) const
    {
        return m_shader_keys[material_index];
    }

    const std::vector<std::string>& Renderer::GetShaderKeywords() const
    {
        return m_shader_keywords;
    }

    void Renderer::UpdateShaderKeywords()
    {
        m_shader_keys.resize(m_materials.size());

        for (int i = 0; i < m_materials.size(); ++i)
        {
            if (m_materials[i] && m_materials[i]->GetShaderName().size() > 0)
            {
                m_shader_keys[i] = m_materials[i]->EnableKeywords(m_shader_keywords);
            }
        }
    }
    
    std::vector<filament::backend::RenderPrimitiveHandle> Renderer::GetPrimitives()
    {
        return std::vector<filament::backend::RenderPrimitiveHandle>();
    }

	void Renderer::Prepare()
	{
		auto& driver = Engine::Instance()->GetDriverApi();
		const auto& materials = this->GetMaterials();

		for (int i = 0; i < materials.size(); ++i)
		{
			auto& material = materials[i];
			if (material)
			{
				material->Prepare();
			}
		}

		if (!m_transform_uniform_buffer)
		{
			m_transform_uniform_buffer = driver.createUniformBuffer(sizeof(RendererUniforms), filament::backend::BufferUsage::DYNAMIC);
		}

        Bounds bounds = this->GetLocalBounds();
        Vector3 bounds_position = bounds.GetCenter();
        Vector3 bounds_size = bounds.GetSize();
        //auto selected_obj = Engine::Instance()->GetEditor()->GetSelectedGameObject();

        m_renderer_uniforms.model_matrix = this->GetTransform()->GetLocalToWorldMatrix();
        m_renderer_uniforms.bounds_matrix = Matrix4x4::TRS(bounds_position, Quaternion::Identity(), bounds_size);
        m_renderer_uniforms.bounds_color = Color(1, 0, 0, 1);
        m_renderer_uniforms.lightmap_scale_offset = m_lightmap_scale_offset;
        m_renderer_uniforms.lightmap_index = Vector4((float) m_lightmap_index);

		void* buffer = driver.allocate(sizeof(RendererUniforms));
		Memory::Copy(buffer, &m_renderer_uniforms, sizeof(RendererUniforms));
		driver.loadUniformBuffer(m_transform_uniform_buffer, filament::backend::BufferDescriptor(buffer, sizeof(RendererUniforms)));
	}
}
