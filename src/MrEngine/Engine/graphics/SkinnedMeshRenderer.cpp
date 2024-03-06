/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "SkinnedMeshRenderer.h"
#include "Entity.h"
#include "Engine.h"
#include "Debug.h"
#include "Transform.h"
#include "string/stringutils.h"

namespace moonriver
{
    SkinnedMeshRenderer::SkinnedMeshRenderer():
		m_blend_shape_dirty(false),
		m_vb_vertex_count(0)
    {

    }

    SkinnedMeshRenderer::~SkinnedMeshRenderer()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        if (m_bones_uniform_buffer)
        {
            driver.destroyUniformBuffer(m_bones_uniform_buffer);
			m_bones_uniform_buffer.clear();
        }
        if (m_blend_shape_sampler_group)
        {
            driver.destroySamplerGroup(m_blend_shape_sampler_group);
            m_blend_shape_sampler_group.clear();
        }

		if (m_vb)
		{
			driver.destroyVertexBuffer(m_vb);
			m_vb.clear();
		}
		for (int i = 0; i < m_primitives.size(); ++i)
		{
			driver.destroyRenderPrimitive(m_primitives[i]);
			m_primitives[i].clear();
		}
		m_primitives.clear();
    }

	void SkinnedMeshRenderer::SetMesh(const std::shared_ptr<Mesh>& mesh)
	{
		MeshRenderer::SetMesh(mesh);

		m_blend_shape_weights.clear();

		auto& driver = Engine::Instance()->GetDriverApi();
		if (m_vb)
		{
			driver.destroyVertexBuffer(m_vb);
			m_vb.clear();
		}
		for (int i = 0; i < m_primitives.size(); ++i)
		{
			driver.destroyRenderPrimitive(m_primitives[i]);
			m_primitives[i].clear();
		}
		m_primitives.clear();
	}

	float SkinnedMeshRenderer::GetBlendShapeWeight(const std::string& name)
	{
		float weight = 0;

		if (m_blend_shape_weights.size() == 0)
		{
			const auto& mesh = this->GetMesh();
			if (mesh)
			{
				const auto& blend_shapes = mesh->GetBlendShapes();
				for (int i = 0; i < blend_shapes.size(); ++i)
				{
					m_blend_shape_weights[blend_shapes[i].name] = { i, 0.0f };
				}
			}
		}

		const BlendShapeWeight* ptr;
        std::map<std::string, BlendShapeWeight>::iterator it = m_blend_shape_weights.find(name);
		if (it != m_blend_shape_weights.end())
		{
            ptr = &it->second;
			weight = ptr->weight;
		}

		return weight;
	}

	void SkinnedMeshRenderer::SetBlendShapeWeight(const std::string& name, float weight)
	{
		if (m_blend_shape_weights.size() == 0)
		{
			const auto& mesh = this->GetMesh();
			if (mesh)
			{
				const auto& blend_shapes = mesh->GetBlendShapes();
				for (int i = 0; i < blend_shapes.size(); ++i)
				{
					m_blend_shape_weights[blend_shapes[i].name] = { i, 0.0f };
				}
			}
		}

		BlendShapeWeight* ptr;
        std::map<std::string, BlendShapeWeight>::iterator it = m_blend_shape_weights.find(name);
        if (it != m_blend_shape_weights.end())
		{
            ptr = &it->second;
			ptr->weight = weight;
			m_blend_shape_dirty = true;
		}
	}

    void SkinnedMeshRenderer::FindBones()
    {
        auto root = m_bones_root.lock();
        const auto& root_name = root->GetName();

        m_bones.resize(m_bone_paths.size());
        for (int i = 0; i < m_bones.size(); ++i)
        {
            if (StartsWiths(m_bone_paths[i], root_name))
            {
                m_bones[i] = root->Find(m_bone_paths[i].substr(root_name.size() + 1));
            }

            if (m_bones[i].expired())
            {
                Log("can not find bone: %s", m_bone_paths[i].c_str());
            }
        }
    }

    void SkinnedMeshRenderer::SetBones(const std::vector<std::shared_ptr<Transform>>& bones)
    {
        m_bones.resize(bones.size());
        for (int i = 0; i < m_bones.size(); ++i)
        {
            m_bones[i] = bones[i];
        }
    }

    void SkinnedMeshRenderer::Prepare()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        const auto& materials = this->GetMaterials();
        const auto& mesh = this->GetMesh();

		// update bones
        if (materials.size() > 0 && mesh && m_bones.size() > 0)
        {
            const auto& bindposes = mesh->GetBindposes();
            int bone_count = bindposes.size();

            assert(m_bones.size() == bone_count);
            assert(m_bones.size() <= SkinnedMeshRendererUniforms::BONES_VECTOR_MAX_COUNT / 3);

            if (m_bones.empty())
            {
                this->FindBones();
            }

			m_bone_vectors.resize(bone_count * 3);

            for (int i = 0; i < bone_count; ++i)
            {
                Matrix4x4 mat = m_bones[i].lock()->GetLocalToWorldMatrix() * bindposes[i].Transpose();

				m_bone_vectors[i * 3 + 0] = mat.GetRow(0);
				m_bone_vectors[i * 3 + 1] = mat.GetRow(1);
				m_bone_vectors[i * 3 + 2] = mat.GetRow(2);
            }

            if (!m_bones_uniform_buffer)
            {
                m_bones_uniform_buffer = driver.createUniformBuffer(sizeof(SkinnedMeshRendererUniforms), filament::backend::BufferUsage::DYNAMIC);
            }

			void* buffer = driver.allocate(m_bone_vectors.size() * sizeof(m_bone_vectors[0]));
            Memory::Copy(buffer, m_bone_vectors.data(), m_bone_vectors.size() * sizeof(m_bone_vectors[0]));
            driver.loadUniformBuffer(m_bones_uniform_buffer, filament::backend::BufferDescriptor(buffer, m_bone_vectors.size() * sizeof(m_bone_vectors[0])));
        }

		// update blend shapes
		if (mesh)
		{
            if (mesh->GetBlendShapeTexture())
            {
                const auto& blend_shape_texture = mesh->GetBlendShapeTexture();
                this->EnableShaderKeyword("BLEND_SHAPE_ON");

                m_bone_vectors.resize(2 + m_blend_shape_weights.size());

                // store weight count in element 0�� texture size in 1
                m_bone_vectors[0] = Vector4((float) m_bone_vectors.size(), (float) mesh->GetVertices().size(), (float) mesh->GetBlendShapes().size());
                m_bone_vectors[1] = Vector4((float) blend_shape_texture->GetWidth(), (float) blend_shape_texture->GetHeight());

                int weight_index = 2;
                for (const auto& i : m_blend_shape_weights)
                {
                    m_bone_vectors[weight_index] = Vector4((float) i.second.index, i.second.weight);
                    weight_index++;
                }

                if (!m_bones_uniform_buffer)
                {
                    m_bones_uniform_buffer = driver.createUniformBuffer(sizeof(SkinnedMeshRendererUniforms), filament::backend::BufferUsage::DYNAMIC);
                }

                void* buffer = driver.allocate(m_bone_vectors.size() * sizeof(m_bone_vectors[0]));
                Memory::Copy(buffer, m_bone_vectors.data(), m_bone_vectors.size() * sizeof(m_bone_vectors[0]));
                driver.loadUniformBuffer(m_bones_uniform_buffer, filament::backend::BufferDescriptor(buffer, m_bone_vectors.size() * sizeof(m_bone_vectors[0])));

                // blend shape sampler
                if (!m_blend_shape_sampler_group)
                {
                    m_blend_shape_sampler_group = driver.createSamplerGroup(1);
                }
                filament::backend::SamplerGroup samplers(1);
                samplers.setSampler(0, blend_shape_texture->GetTexture(), blend_shape_texture->GetSampler());
                driver.updateSamplerGroup(m_blend_shape_sampler_group, std::move(samplers));
            }
            else if (m_blend_shape_dirty)
            {
                m_blend_shape_dirty = false;

                const auto& vertices = mesh->GetVertices();
                const auto& submeshes = mesh->GetSubmeshes();
                const auto& blend_shapes = mesh->GetBlendShapes();

                Mesh::Vertex* buffer = (Mesh::Vertex*) driver.allocate(vertices.size() * sizeof(vertices[0]));
                Memory::Copy(buffer, vertices.data(), vertices.size() * sizeof(vertices[0]));

                for (const auto& i : m_blend_shape_weights)
                {
                    if (i.second.weight > 0)
                    {
                        const auto& shape = blend_shapes[i.second.index];

                        for (int j = 0; j < vertices.size(); ++j)
                        {
                            const auto& frame = shape.frame;

                            buffer[j].vertex += frame.vertices[j] * i.second.weight;
                            buffer[j].normal += frame.normals[j] * i.second.weight;
                            buffer[j].tangent += frame.tangents[j] * i.second.weight;
                        }
                    }
                }

                if (m_vb_vertex_count != vertices.size())
                {
                    if (m_vb)
                    {
                        driver.destroyVertexBuffer(m_vb);
                        m_vb.clear();
                    }
                }
                if (!m_vb)
                {
                    m_vb = driver.createVertexBuffer(1, (uint8_t) Shader::AttributeLocation::Count, vertices.size(), mesh->GetAttributes(), filament::backend::BufferUsage::DYNAMIC);
                    m_vb_vertex_count = vertices.size();
                }

                if (m_submeshes.size() != submeshes.size() ||
                    Memory::Compare(&m_submeshes[0], &submeshes[0], submeshes.size() * sizeof(submeshes[0])) != 0)
                {
                    for (int i = 0; i < m_primitives.size(); ++i)
                    {
                        driver.destroyRenderPrimitive(m_primitives[i]);
                        m_primitives[i].clear();
                    }
                    m_primitives.clear();
                    m_submeshes.clear();
                }
                if (m_primitives.size() == 0)
                {
                    m_primitives.resize(submeshes.size());
                    for (int i = 0; i < m_primitives.size(); ++i)
                    {
                        m_primitives[i] = driver.createRenderPrimitive();

                        driver.setRenderPrimitiveBuffer(m_primitives[i], m_vb, mesh->GetIndexBuffer(), mesh->GetEnabledAttributes());
                        driver.setRenderPrimitiveRange(m_primitives[i], filament::backend::PrimitiveType::TRIANGLES, submeshes[i].index_first, 0, vertices.size() - 1, submeshes[i].index_count);
                    }
                    m_submeshes = submeshes;
                }

                driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(buffer, vertices.size() * sizeof(vertices[0])), 0);
            }
		}

        MeshRenderer::Prepare();
    }

    std::vector<filament::backend::RenderPrimitiveHandle> SkinnedMeshRenderer::GetPrimitives()
    {
        std::vector<filament::backend::RenderPrimitiveHandle> primitives;

		if (m_primitives.size() > 0)
		{
			primitives = m_primitives;
		}
		else
		{
			const auto& mesh = this->GetMesh();
			if (mesh)
			{
				primitives = mesh->GetPrimitives();
			}
		}

        return primitives;
    }
}
