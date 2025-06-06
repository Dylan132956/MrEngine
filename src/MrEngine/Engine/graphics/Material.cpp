#include "Material.h"
#include "Engine.h"

namespace moonriver
{
    std::shared_ptr<Material> Material::m_shared_bounds_material;

    Material::Material()
    {
        TextureIds.fill(-1);
    }

    void Material::Done()
    {
        m_shared_bounds_material.reset();
    }

    Material::Material(const std::shared_ptr<Shader>& shader) :
        m_scissor_rect(0, 0, 1, 1)
    {
        TextureIds.fill(-1);
        ShaderVariant variant;
        variant.key = shader->GetShaderKey();
        variant.keywords = shader->GetKeywords();
        variant.shader = shader;
        m_shader_variants[variant.key] = variant;

        m_unifrom_buffers.resize(shader->GetPassCount());
        for (int i = 0; i < m_unifrom_buffers.size(); ++i)
        {
            m_unifrom_buffers[i].resize((int)Shader::BindingPoint::Count);
        }

        m_samplers.resize(shader->GetPassCount());
        for (int i = 0; i < m_samplers.size(); ++i)
        {
            m_samplers[i].resize((int)Shader::BindingPoint::Count);
        }
        m_pbr_uniform_buffers.resize(shader->GetPassCount());

        this->SetTexture(MaterialProperty::TEXTURE, Texture::GetSharedWhiteTexture());
        this->SetVector(MaterialProperty::TEXTURE_SCALE_OFFSET, Vector4(1, 1, 0, 0));
        this->SetColor(MaterialProperty::COLOR, Color(1, 1, 1, 1));

        std::string shaderName = shader->GetName();
        if (shaderName == "standard") {
            m_isPbr = true;
        }
        else
        {
            m_isPbr = false;
        }
    }

    Material::~Material()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        for (int i = 0; i < m_unifrom_buffers.size(); ++i)
        {
            for (int j = 0; j < m_unifrom_buffers[i].size(); ++j)
            {
                if (m_unifrom_buffers[i][j].uniform_buffer)
                {
                    driver.destroyUniformBuffer(m_unifrom_buffers[i][j].uniform_buffer);
                    m_unifrom_buffers[i][j].uniform_buffer.clear();
                }
            }
        }
        m_unifrom_buffers.clear();

        for (int i = 0; i < m_samplers.size(); ++i)
        {
            for (int j = 0; j < m_samplers[i].size(); ++j)
            {
                if (m_samplers[i][j].sampler_group)
                {
                    driver.destroySamplerGroup(m_samplers[i][j].sampler_group);
                    m_samplers[i][j].sampler_group.clear();
                }
            }
        }
        m_samplers.clear();

        for (int i=0; i< m_pbr_uniform_buffers.size(); ++i)
        {
			driver.destroyUniformBuffer(m_pbr_uniform_buffers[i]);
			m_pbr_uniform_buffers[i].clear();
        }
    }

    const std::string& Material::GetShaderName()
    {
        return m_shader_variants.begin()->second.shader->GetName();
    }

    const std::shared_ptr<Material>& Material::GetSharedBoundsMaterial()
    {
        if (!m_shared_bounds_material)
        {
            m_shared_bounds_material = std::make_shared<Material>(Shader::Find("Bounds"));
        }

        return m_shared_bounds_material;
    }

	void Material::SetScissorRect(const Rect& rect)
	{
		m_scissor_rect = rect;
	}

    std::string Material::EnableKeywords(const std::vector<std::string>& keywords)
    {
        std::string key = Shader::MakeKey(this->GetShaderName(), keywords);
        if (m_shader_variants.count(key) == 0)
        {
            auto shader = Shader::Find(this->GetShaderName(), keywords);

            ShaderVariant variant;
            variant.key = shader->GetShaderKey();
            variant.keywords = shader->GetKeywords();
            variant.shader = shader;
            m_shader_variants[variant.key] = variant;
        }
        return key;
    }

    void Material::Prepare(int pass)
    {
        for (auto& i : m_properties)
        {
            if (i.second.dirty)
            {
                i.second.dirty = false;

                switch (i.second.type)
                {
                case MaterialProperty::Type::Texture:
                    this->UpdateUniformTexture(i.first, i.second.texture);
                    break;
                case MaterialProperty::Type::VectorArray:
                    this->UpdateUniformMember(i.first, i.second.vector_array.data(), i.second.vector_array.size() * sizeof(i.second.vector_array[0]));
                    break;
                case MaterialProperty::Type::MatrixArray:
                    this->UpdateUniformMember(i.first, i.second.matrix_array.data(), i.second.matrix_array.size() * sizeof(i.second.matrix_array[0]));
                    break;
                default:
                    this->UpdateUniformMember(i.first, &i.second.data, i.second.size);
                    break;
                }
            }
        }

        auto& driver = Engine::Instance()->GetDriverApi();

        for (int i = 0; i < m_unifrom_buffers.size(); ++i)
        {
            auto& unifrom_buffers = m_unifrom_buffers[i];

            if (pass >= 0 && pass != i)
            {
                continue;
            }

            for (int j = 0; j < unifrom_buffers.size(); ++j)
            {
                auto& unifrom_buffer = unifrom_buffers[j];

                if (unifrom_buffer.dirty)
                {
                    unifrom_buffer.dirty = false;

                    void* buffer = Memory::Alloc<void>(unifrom_buffer.buffer.Size());
                    Memory::Copy(buffer, unifrom_buffer.buffer.Bytes(), unifrom_buffer.buffer.Size());
                    driver.loadUniformBuffer(unifrom_buffer.uniform_buffer, filament::backend::BufferDescriptor(buffer, unifrom_buffer.buffer.Size(), FreeBufferCallback));
                }
            }
        }

        for (int i = 0; i < m_samplers.size(); ++i)
        {
            auto& sampler_groups = m_samplers[i];

            if (pass >= 0 && pass != i)
            {
                continue;
            }

            for (int j = 0; j < sampler_groups.size(); ++j)
            {
                auto& sampler_group = sampler_groups[j];

                if (sampler_group.dirty)
                {
                    sampler_group.dirty = false;

                    filament::backend::SamplerGroup samplers(sampler_group.samplers.size());
                    for (int k = 0; k < sampler_group.samplers.size(); ++k)
                    {
                        const auto& sampler = sampler_group.samplers[k];
                        if (sampler.texture)
                        {
                            samplers.setSampler(k, sampler.texture->GetTexture(), sampler.texture->GetSampler());
                        }
                    }
                    driver.updateSamplerGroup(sampler_group.sampler_group, std::move(samplers));
                }
            }
        }
        if (m_isPbr) {
            UpdatePbrUniform();
        }
    }

    void Material::UpdateUniformMember(const std::string& name, const void* data, int size)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        const auto& shader = m_shader_variants.begin()->second.shader;

        for (int i = 0; i < shader->GetPassCount(); ++i)
        {
            const auto& pass = shader->GetPass(i);

            for (int j = 0; j < pass.uniforms.size(); ++j)
            {
                const auto& uniform = pass.uniforms[j];
                bool find = false;

                for (int k = 0; k < uniform.members.size(); ++k)
                {
                    const auto& member = uniform.members[k];

                    if (name == member.name)
                    {
                        auto& unifrom_buffer = m_unifrom_buffers[i][uniform.binding];

                        if (!unifrom_buffer.uniform_buffer)
                        {
                            unifrom_buffer.uniform_buffer = driver.createUniformBuffer(uniform.size, filament::backend::BufferUsage::DYNAMIC);
                            unifrom_buffer.buffer = ByteBuffer(uniform.size);
                        }

                        assert(size <= member.size);

                        Memory::Copy(&unifrom_buffer.buffer[member.offset], data, size);

                        unifrom_buffer.dirty = true;

                        find = true;
                        break;
                    }
                }

                if (find)
                {
                    break;
                }
            }
        }
    }

    void Material::UpdateUniformTexture(const std::string& name, const std::shared_ptr<Texture>& texture)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        const auto& shader = m_shader_variants.begin()->second.shader;

        for (int i = 0; i < shader->GetPassCount(); ++i)
        {
            const auto& pass = shader->GetPass(i);

            for (int j = 0; j < pass.samplers.size(); ++j)
            {
                const auto& group = pass.samplers[j];

                for (int k = 0; k < group.samplers.size(); ++k)
                {
                    const auto& sampler = group.samplers[k];

                    if (name == sampler.name)
                    {
                        auto& sampler_group = m_samplers[i][group.binding];

                        if (!sampler_group.sampler_group)
                        {
                            sampler_group.sampler_group = driver.createSamplerGroup(group.samplers.size());
                            sampler_group.samplers.resize(group.samplers.size());
                        }

                        sampler_group.samplers[k].binding = sampler.binding;
                        sampler_group.samplers[k].texture = texture;

                        sampler_group.dirty = true;
                    }
                }
            }
        }
    }

    void Material::UpdatePbrUniform()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        for (int i=0; i< m_pbr_uniform_buffers.size(); ++i)
        {
			if (!m_pbr_uniform_buffers[i])
			{
				m_pbr_uniform_buffers[i] = driver.createUniformBuffer(sizeof(ShaderAttribs), filament::backend::BufferUsage::DYNAMIC);
			}
			void* buffer = driver.allocate(sizeof(ShaderAttribs));
			Memory::Copy(buffer, &Attribs, sizeof(ShaderAttribs));
			driver.loadUniformBuffer(m_pbr_uniform_buffers[i], filament::backend::BufferDescriptor(buffer, sizeof(ShaderAttribs)));

            if (Engine::Instance()->GetShaderModel() == filament::backend::ShaderModel::GL_ES_20)
            {
				SetVector("fs.PerMaterialFragment.g_MaterialInfo.BaseColorFactor", Attribs.BaseColorFactor);
				SetVector("fs.PerMaterialFragment.g_MaterialInfo.EmissiveFactor", Attribs.EmissiveFactor);
				SetVector("fs.PerMaterialFragment.g_MaterialInfo.SpecularFactor", Attribs.SpecularFactor);
				SetVector("fs.PerMaterialFragment.g_MaterialInfo.MroFactor", Vector4(Attribs.NormalSlice, Attribs.OcclusionSlice, Attribs.EmissiveSlice, Attribs.MetallicFactor));
				SetVector("fs.PerMaterialFragment.g_MaterialInfo.RacFactor", Vector4(Attribs.RoughnessFactor, Attribs.AlphaMode, Attribs.AlphaCutoff, Attribs.Dummy0));
            }
            //SetVector("_242.g_MaterialInfo.SpecularFactor", Vector4(Attribs.MetallicFactor, Attribs.RoughnessFactor, Attribs.AlphaMode, Attribs.));
        }
    }

    void Material::SetScissor(int target_width, int target_height)
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        // set scissor
        int32_t scissor_left = (int32_t)(m_scissor_rect.x * target_width);
        int32_t scissor_bottom = (int32_t)((1.0f - (m_scissor_rect.y + m_scissor_rect.h)) * target_height);
        uint32_t scissor_width = (uint32_t)(m_scissor_rect.w * target_width);
        uint32_t scissor_height = (uint32_t)(m_scissor_rect.h * target_height);
        driver.setViewportScissor(scissor_left, scissor_bottom, scissor_width, scissor_height);
    }

	std::vector<std::string> splitByTwoDots(const std::string& str) {
		std::vector<std::string> parts(3); // 默认初始化3个空字符串
		size_t first_dot = str.find('.');

		// 提取第一段
		if (first_dot != std::string::npos) {
			parts[0] = str.substr(0, first_dot);
			size_t second_dot = str.find('.', first_dot + 1); // 从第一个点后开始找

			// 提取第二段
			if (second_dot != std::string::npos) {
				parts[1] = str.substr(first_dot + 1, second_dot - (first_dot + 1));
				parts[2] = str.substr(second_dot + 1); // 第三段为剩余部分
			}
			else {
				parts[1] = str.substr(first_dot + 1); // 第二个点不存在，第二段取到末尾
			}
		}
		else {
			parts[0] = str; // 无点，整个字符串作为第一段
		}
		return parts;
	}

    void Material::Bind(const std::shared_ptr<Shader>& shader, int pass)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        const auto& unifrom_buffers = m_unifrom_buffers[pass];
        const auto& samplers = m_samplers[pass];

        // bind uniforms
        for (int i = 0; i < unifrom_buffers.size(); ++i)
        {
            if (unifrom_buffers[i].uniform_buffer)
            {
                driver.bindUniformBuffer((size_t)i, unifrom_buffers[i].uniform_buffer);
            }
        }

        // bind samplers
        for (int i = 0; i < samplers.size(); ++i)
        {
            if (samplers[i].sampler_group)
            {
                driver.bindSamplers((size_t)i, samplers[i].sampler_group);
            }
        }

		if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL &&
			Engine::Instance()->GetShaderModel() == filament::backend::ShaderModel::GL_ES_20)
        {

			auto GetShaderParamName = [](const Shader::Pass& pass, const std::string& inputName)
			{
				// 1. 添加输入有效性检查
				std::vector<std::string> vstring = splitByTwoDots(inputName);
				if (vstring.size() < 3) { // 确保分割后的三段都存在
					assert(!"Invalid uniform name format");
					return std::string();
				}

				// 2. 统一使用 find 方法处理 const 容器
				const auto& vsMap = pass.vs_mapUniformsName;
				const auto& fsMap = pass.fs_mapUniformsName;

				if (vstring[0] == "vs") {
					auto it = vsMap.find(vstring[1]);
					if (it != vsMap.end()) {
						return it->second + "." + vstring[2];
					}
					// 3. 添加明确的错误处理
					assert(!"VS uniform not found");
					return std::string();
				}
				else if (vstring[0] == "fs") {
					auto it = fsMap.find(vstring[1]);
					if (it != fsMap.end()) {
						return it->second + "." + vstring[2];
					}
					assert(!"FS uniform not found");
					return std::string();
				}

				// 4. 优化断言信息
				//assert(!"Invalid shader stage prefix (must be vs/fs)");
				return std::string(inputName);
			};

            for (auto& i : m_properties)
            {
                switch (i.second.type)
                {
                case MaterialProperty::Type::Matrix:
                {
                    void* buffer = driver.allocate(sizeof(Matrix4x4));
                    Memory::Copy(buffer, &i.second.data, sizeof(Matrix4x4));
                    driver.setUniformMatrix(
                        shader->GetPass(pass).pipeline.program,
                        GetShaderParamName((shader->GetPass(pass)), (i.first)),
                        1,
                        filament::backend::BufferDescriptor(buffer, sizeof(Matrix4x4)));
                    break;
                }
                case MaterialProperty::Type::Vector:
                case MaterialProperty::Type::Color:
                {
                    void* buffer = driver.allocate(sizeof(Vector4));
                    Memory::Copy(buffer, &i.second.data, sizeof(Vector4));
                    driver.setUniformVector(
                        shader->GetPass(pass).pipeline.program,
                        GetShaderParamName((shader->GetPass(pass)), (i.first)),
                        1,
                        filament::backend::BufferDescriptor(buffer, sizeof(Vector4)));
                    break;
                }
                case MaterialProperty::Type::VectorArray:
                {
                    const auto& array = i.second.vector_array;
                    void* buffer = driver.allocate(sizeof(array[0]) * array.size());
                    Memory::Copy(buffer, &array[0], sizeof(array[0]) * array.size());
                    driver.setUniformVector(
                        shader->GetPass(pass).pipeline.program,
                        GetShaderParamName((shader->GetPass(pass)), (i.first)),
                        array.size(),
                        filament::backend::BufferDescriptor(buffer, sizeof(array[0]) * array.size()));
                    break;
                }
                default:
                    break;
                }
            }
        }
        if (m_isPbr) {
            driver.bindUniformBuffer((size_t)Shader::BindingPoint::PerMaterialFragment, m_pbr_uniform_buffers[pass]);
        }
    }

    const std::shared_ptr<Shader>& Material::GetShader()
    {
        return m_shader_variants.begin()->second.shader;
    }

    const std::shared_ptr<Shader>& Material::GetShader(const std::string& key)
    {
        assert(m_shader_variants.count(key) > 0);
        return m_shader_variants[key].shader;
    }

    const Ref<Shader>& Material::GetShader(const std::vector<std::string>& keywords)
    {
        if (this->GetShaderName().size() > 0)
        {
            auto key = this->EnableKeywords(keywords);
            return this->GetShader(key);
        }
        else
        {
            return this->GetShader();
        }
    }

    int Material::GetQueue() const
    {
        if (m_queue)
        {
            return *m_queue;
        }

        return m_shader_variants.begin()->second.shader->GetQueue();
    }

    void Material::SetQueue(int queue)
    {
        m_queue = std::make_shared<int>(queue);
    }

    const Matrix4x4* Material::GetMatrix(const std::string& name) const
    {
        return this->GetProperty<Matrix4x4>(name, MaterialProperty::Type::Matrix);
    }

    void Material::SetMatrix(const std::string& name, const Matrix4x4& value)
    {
        this->SetProperty(name, value, MaterialProperty::Type::Matrix);
    }

    const Vector4* Material::GetVector(const std::string& name) const
    {
        return this->GetProperty<Vector4>(name, MaterialProperty::Type::Vector);
    }

    void Material::SetVector(const std::string& name, const Vector4& value)
    {
        this->SetProperty(name, value, MaterialProperty::Type::Vector);
    }

    void Material::SetColor(const std::string& name, const Color& value)
    {
        this->SetProperty(name, value, MaterialProperty::Type::Color);
    }

    void Material::SetFloat(const std::string& name, float value)
    {
        this->SetProperty(name, value, MaterialProperty::Type::Float);
    }

    void Material::SetInt(const std::string& name, int value)
    {
        this->SetProperty(name, value, MaterialProperty::Type::Int);
    }

    Ref<Texture> Material::GetTexture(const std::string& name) const
    {
        Ref<Texture> texture;
        const MaterialProperty* property_ptr;

        const_itProperty it = m_properties.find(name);
        if (it != m_properties.end())
        {
            property_ptr = &it->second;
            if (property_ptr->type == MaterialProperty::Type::Texture)
            {
                texture = property_ptr->texture;
            }
        }
        return texture;
    }

    void Material::SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture)
    {
        MaterialProperty* property_ptr;
        itProperty it = m_properties.find(name);
        if (it != m_properties.end())
        {
            property_ptr = &it->second;
            property_ptr->type = MaterialProperty::Type::Texture;
            property_ptr->texture = texture;
            property_ptr->dirty = true;
        }
        else
        {
            MaterialProperty property;
            property.name = name;
            property.type = MaterialProperty::Type::Texture;
            property.texture = texture;
            property.dirty = true;
            m_properties[name] = property;
        }
    }

    void Material::SetVectorArray(const std::string& name, const std::vector<Vector4>& array)
    {
        MaterialProperty* property_ptr;
        itProperty it = m_properties.find(name);
        if (it != m_properties.end())
        {
            property_ptr = &it->second;
            property_ptr->type = MaterialProperty::Type::VectorArray;
            property_ptr->vector_array = array;
            property_ptr->dirty = true;
        }
        else
        {
            MaterialProperty property;
            property.name = name;
            property.type = MaterialProperty::Type::VectorArray;
            property.vector_array = array;
            property.dirty = true;
            m_properties[name] = property;
        }
    }

    void Material::SetMatrixArray(const std::string& name, const std::vector<Matrix4x4>& array)
    {
        MaterialProperty* property_ptr;
        itProperty it = m_properties.find(name);
        if (it != m_properties.end())
        {
            property_ptr = &it->second;
            property_ptr->type = MaterialProperty::Type::MatrixArray;
            property_ptr->matrix_array = array;
            property_ptr->dirty = true;
        }
        else
        {
            MaterialProperty property;
            property.name = name;
            property.type = MaterialProperty::Type::MatrixArray;
            property.matrix_array = array;
            property.dirty = true;
            m_properties[name] = property;
        }
    }
}
