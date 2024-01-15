
#include "Mesh.h"
#include "Debug.h"
#include "Engine.h"
#include "Shader.h"
#include "Texture.h"
#include "io/File.h"
#include "io/MemoryStream.h"
#include "memory/Memory.h"
#include "Debug.h"

namespace moonriver
{
	std::shared_ptr<Mesh> Mesh::m_shared_quad_mesh;
    std::shared_ptr<Mesh> Mesh::m_shared_bounds_mesh;

	void Mesh::Init()
	{
        Mesh::GetSharedQuadMesh();
        Mesh::GetSharedBoundsMesh();
	}

	void Mesh::Done()
	{
		m_shared_quad_mesh.reset();
        m_shared_bounds_mesh.reset();
	}

	const std::shared_ptr<Mesh>& Mesh::GetSharedQuadMesh()
	{
		if (!m_shared_quad_mesh)
		{
			std::vector<Mesh::Vertex> vertices(4);
			vertices[0].vertex = Vector3(-1, 1, 0);
			vertices[1].vertex = Vector3(-1, -1, 0);
			vertices[2].vertex = Vector3(1, -1, 0);
			vertices[3].vertex = Vector3(1, 1, 0);
			vertices[0].uv = Vector2(0, 0);
			vertices[1].uv = Vector2(0, 1);
			vertices[2].uv = Vector2(1, 1);
			vertices[3].uv = Vector2(1, 0);
			std::vector<unsigned int> indices = {
				0, 1, 2, 0, 2, 3
			};
			m_shared_quad_mesh = std::make_shared<Mesh>(std::move(vertices), std::move(indices));
		}

		return m_shared_quad_mesh;
	}

    const std::shared_ptr<Mesh>& Mesh::GetSharedBoundsMesh()
    {
        if (!m_shared_bounds_mesh)
        {
            std::vector<Mesh::Vertex> vertices(8);
            vertices[0].vertex = Vector3(-0.5f, 0.5f, -0.5f);
            vertices[1].vertex = Vector3(-0.5f, -0.5f, -0.5f);
            vertices[2].vertex = Vector3(0.5f, -0.5f, -0.5f);
            vertices[3].vertex = Vector3(0.5f, 0.5f, -0.5f);
            vertices[4].vertex = Vector3(-0.5f, 0.5f, 0.5f);
            vertices[5].vertex = Vector3(-0.5f, -0.5f, 0.5f);
            vertices[6].vertex = Vector3(0.5f, -0.5f, 0.5f);
            vertices[7].vertex = Vector3(0.5f, 0.5f, 0.5f);
            std::vector<unsigned int> indices = {
                0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7,
            };
            m_shared_bounds_mesh = std::make_shared<Mesh>(std::move(vertices), std::move(indices),
                std::vector<Submesh>(), false, false, filament::backend::PrimitiveType::LINES);
        }

        return m_shared_bounds_mesh;
    }

    std::shared_ptr<Mesh> Mesh::LoadFromFile(const std::string& path)
    {
        std::shared_ptr<Mesh> mesh;

        if (File::Exist(path))
        {
            MemoryStream ms(File::ReadAllBytes(path));

            int name_size = ms.Read<int>();
            std::string mesh_name = ms.ReadString(name_size);

            std::vector<Vertex>* vertices = new std::vector<Vertex>();
            std::vector<unsigned int>* indices = new std::vector<unsigned int>();
            std::vector<Submesh>* submeshes = new std::vector<Submesh>();
            std::vector<Matrix4x4>* bindposes = new std::vector<Matrix4x4>();
            std::vector<BlendShape>* blend_shapes = new std::vector<BlendShape>();
            
            int vertex_count = ms.Read<int>();
            vertices->resize(vertex_count);

            for (int i = 0; i < vertex_count; ++i)
            {
                (*vertices)[i].vertex = ms.Read<Vector3>();
            }

            int color_count = ms.Read<int>();
            for (int i = 0; i < color_count; ++i)
            {
                float r = ms.Read<byte>() / 255.0f;
                float g = ms.Read<byte>() / 255.0f;
                float b = ms.Read<byte>() / 255.0f;
                float a = ms.Read<byte>() / 255.0f;
                (*vertices)[i].color = Color(r, g, b, a);
            }

            int uv_count = ms.Read<int>();
            for (int i = 0; i < uv_count; ++i)
            {
                (*vertices)[i].uv = ms.Read<Vector2>();
            }

            int uv2_count = ms.Read<int>();
            for (int i = 0; i < uv2_count; ++i)
            {
                (*vertices)[i].uv2 = ms.Read<Vector2>();
            }

            int normal_count = ms.Read<int>();
            for (int i = 0; i < normal_count; ++i)
            {
                (*vertices)[i].normal = ms.Read<Vector3>();
            }

            int tangent_count = ms.Read<int>();
            for (int i = 0; i < tangent_count; ++i)
            {
                (*vertices)[i].tangent = ms.Read<Vector4>();
            }

            int bone_weight_count = ms.Read<int>();
            for (int i = 0; i < bone_weight_count; ++i)
            {
                (*vertices)[i].bone_weights = ms.Read<Vector4>();
                float index0 = (float) ms.Read<byte>();
                float index1 = (float) ms.Read<byte>();
                float index2 = (float) ms.Read<byte>();
                float index3 = (float) ms.Read<byte>();
                (*vertices)[i].bone_indices = Vector4(index0, index1, index2, index3);
            }

            int index_count = ms.Read<int>();
            indices->resize(index_count);
            for (int i = 0; i < index_count; ++i)
            {
                (*indices)[i] = ms.Read<unsigned short>();
            }

            int submesh_count = ms.Read<int>();
            submeshes->resize(submesh_count);
            ms.Read(&(*submeshes)[0], submeshes->size() * sizeof(submeshes[0]));

            int bindpose_count = ms.Read<int>();
            if (bindpose_count > 0)
            {
                bindposes->resize(bindpose_count);
                ms.Read(&(*bindposes)[0], bindposes->size() * sizeof(bindposes[0]));
            }
            
            int blend_shape_count = ms.Read<int>();
            if (blend_shape_count > 0)
            {
                blend_shapes->resize(blend_shape_count);

                for (int i = 0; i < blend_shape_count; ++i)
                {
                    auto& shape = (*blend_shapes)[i];
                    
                    int string_size = ms.Read<int>();
                    std::string shape_name = ms.ReadString(string_size);
                    int frame_count = ms.Read<int>();
                    
                    shape.name = shape_name;
                    auto& frame = shape.frame;

                    if (frame_count > 0)
                    {
                        frame.vertices.resize(vertex_count, Vector3::Zero());
                        frame.normals.resize(normal_count, Vector3::Zero());
                        frame.tangents.resize(tangent_count, Vector3::Zero());
                    }

                    for (int j = 0; j < frame_count; ++j)
                    {
                        float weight = ms.Read<float>() / 100.0f;

                        for (int k = 0; k < vertex_count; ++k)
                        {
                            Vector3 vertex = ms.Read<Vector3>();
                            frame.vertices[k] += vertex * weight;
                        }

                        for (int k = 0; k < normal_count; ++k)
                        {
                            Vector3 normal = ms.Read<Vector3>();
                            frame.normals[k] += normal * weight;
                        }

                        for (int k = 0; k < tangent_count; ++k)
                        {
                            Vector3 tangent = ms.Read<Vector3>();
                            frame.tangents[k] += tangent * weight;
                        }
                    }
                }
            }
            
            Vector3 bounds_center = ms.Read<Vector3>();
            Vector3 bounds_size = ms.Read<Vector3>();

            mesh = RefMake<Mesh>(std::move(*vertices), std::move(*indices), *submeshes);
            mesh->SetName(mesh_name);
            mesh->SetBindposes(std::move(*bindposes));
            mesh->SetBlendShapes(std::move(*blend_shapes));
            mesh->m_bounds = Bounds(bounds_center - bounds_size / 2, bounds_center + bounds_size / 2);

            // create blendshape texture
            if (blend_shape_count > 0 && Texture::SelectFormat({ TextureFormat::R32G32B32A32F }, false) != TextureFormat::None)
            {
                int vector_count = vertex_count * blend_shape_count + normal_count * blend_shape_count + tangent_count * blend_shape_count;
                const int blend_shape_texture_width = 2048;
                int blend_shape_texture_height = vector_count / blend_shape_texture_width;
                if (vector_count % blend_shape_texture_width != 0)
                {
                    blend_shape_texture_height += 1;
                }
                assert(blend_shape_texture_height <= 2048);
                vector_count = blend_shape_texture_height * blend_shape_texture_width;

                ByteBuffer blend_shape_texture_buffer(vector_count * sizeof(Vector4));
                Memory::Zero(blend_shape_texture_buffer.Bytes(), blend_shape_texture_buffer.Size());
                Vector4* pvector = (Vector4*) blend_shape_texture_buffer.Bytes();
                for (int i = 0; i < blend_shape_count; ++i)
                {
                    for (int j = 0; j < vertex_count; ++j)
                    {
                        pvector[i * vertex_count + j] = mesh->m_blend_shapes[i].frame.vertices[j];
                    }

                    for (int j = 0; j < normal_count; ++j)
                    {
                        pvector[vertex_count * blend_shape_count + i * normal_count + j] = mesh->m_blend_shapes[i].frame.normals[j];
                    }

                    for (int j = 0; j < tangent_count; ++j)
                    {
                        pvector[vertex_count * blend_shape_count + normal_count * blend_shape_count + i * tangent_count + j] = mesh->m_blend_shapes[i].frame.tangents[j];
                    }
                }
                
                mesh->m_blend_shape_texture = Texture::CreateTexture2DFromMemory(
                    blend_shape_texture_buffer,
                    blend_shape_texture_width,
                    blend_shape_texture_height,
                    TextureFormat::R32G32B32A32F,
                    FilterMode::Nearest,
                    SamplerAddressMode::ClampToEdge,
                    false);
            }

            delete vertices;
            delete indices;
            delete submeshes;
            delete bindposes;
            delete blend_shapes;
        }
        else
        {
            Log("mesh file not exist: %s", path.c_str());
        }

        return mesh;
    }

    Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, const std::vector<Submesh>& submeshes, bool uint32_index, bool dynamic, filament::backend::PrimitiveType primitive_type):
        m_buffer_vertex_count(vertices.size()),
        m_buffer_index_count(indices.size()),
        m_uint32_index(uint32_index),
		m_enabled_attributes(0),
        m_primitive_type(primitive_type)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        
        m_submeshes = submeshes;
        if (m_submeshes.empty())
        {
            m_submeshes.push_back(Submesh({ 0, indices.size() }));
        }
        
        filament::backend::BufferUsage usage;
        if (dynamic)
        {
            usage = filament::backend::BufferUsage::DYNAMIC;
        }
        else
        {
            usage = filament::backend::BufferUsage::STATIC;
        }
        
        int sizes[] = {
            sizeof(Vertex::vertex), sizeof(Vertex::color), sizeof(Vertex::uv), sizeof(Vertex::uv2),
            sizeof(Vertex::normal), sizeof(Vertex::tangent), sizeof(Vertex::bone_weights), sizeof(Vertex::bone_indices)
        };
        //static const input_element_desc Semantics
        unsigned int semantics[] = {
            0, 4, 10, 11, 1, 2, 9, 8
        };
        filament::backend::ElementType types[] = {
            filament::backend::ElementType::FLOAT4,
            filament::backend::ElementType::FLOAT4,
            filament::backend::ElementType::FLOAT2,
            filament::backend::ElementType::FLOAT2,
            filament::backend::ElementType::FLOAT3,
            filament::backend::ElementType::FLOAT4,
            filament::backend::ElementType::FLOAT4,
            filament::backend::ElementType::FLOAT4
        };
        int offset = 0;
        
        for (int i = 0; i < (int) Shader::AttributeLocation::Count; ++i)
        {
			m_attributes[i].offset = offset;
			m_attributes[i].stride = sizeof(Vertex);
			m_attributes[i].buffer = 0;
			m_attributes[i].type = types[i];
			m_attributes[i].flags = 0;
            m_attributes[i].Semantic = semantics[i];
            
            offset += sizes[i];
        }
        
        m_vb = driver.createVertexBuffer(1, (uint8_t) Shader::AttributeLocation::Count, vertices.size(), m_attributes, usage);

        filament::backend::ElementType index_type;
        if (uint32_index)
        {
            index_type = filament::backend::ElementType::UINT;
        }
        else
        {
            index_type = filament::backend::ElementType::USHORT;
        }
        
        m_ib = driver.createIndexBuffer(index_type, indices.size(), usage);
        
        Mesh::Update(std::move(vertices), std::move(indices), submeshes);
    }
    
    Mesh::~Mesh()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        
		driver.destroyVertexBuffer(m_vb);
		m_vb.clear();

		driver.destroyIndexBuffer(m_ib);
		m_ib.clear();

		for (int i = 0; i < m_primitives.size(); ++i)
		{
			driver.destroyRenderPrimitive(m_primitives[i]);
			m_primitives[i].clear();
		}
		m_primitives.clear();
    }

    void Mesh::Update(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, const std::vector<Submesh>& submeshes)
    {
        auto& driver = Engine::Instance()->GetDriverApi();
     
        m_vertices = std::move(vertices);
        m_indices = std::move(indices);

        // set vertex index in w
        for (int i = 0; i < m_vertices.size(); ++i)
        {
            m_vertices[i].vertex.w = (float)i;
        }
        
        assert(m_vertices.size() <= m_buffer_vertex_count);
        assert(m_indices.size() <= m_buffer_index_count);
        
        m_submeshes = submeshes;
        if (m_submeshes.empty())
        {
            m_submeshes.push_back(Submesh({ 0, m_indices.size() }));
        }
        
        void* buffer = Memory::Alloc<void>(m_vertices.size() * sizeof(m_vertices[0]));
        Memory::Copy(buffer, m_vertices.data(), m_vertices.size() * sizeof(m_vertices[0]));
        driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(buffer, m_vertices.size() * sizeof(m_vertices[0]), FreeBufferCallback), 0);
    
        if (m_uint32_index)
        {
            buffer = Memory::Alloc<void>(m_indices.size() * sizeof(m_indices[0]));
            Memory::Copy(buffer, m_indices.data(), m_indices.size() * sizeof(m_indices[0]));
            driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(buffer, m_indices.size() * sizeof(m_indices[0]), FreeBufferCallback), 0);
        }
        else
        {
            int size = sizeof(unsigned short) * m_indices.size();
            unsigned short* indices_uint16 = Memory::Alloc<unsigned short>(size);
            for (int i = 0; i < m_indices.size(); ++i)
            {
                indices_uint16[i] = m_indices[i];
            }
            driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(indices_uint16, size, FreeBufferCallback), 0);
        }
        
		m_enabled_attributes =
            (1 << (int) Shader::AttributeLocation::Vertex) |
            (1 << (int) Shader::AttributeLocation::Color) |
            (1 << (int) Shader::AttributeLocation::UV) |
            (1 << (int) Shader::AttributeLocation::UV2) |
            (1 << (int) Shader::AttributeLocation::Normal) |
            (1 << (int) Shader::AttributeLocation::Tangent) |
            (1 << (int) Shader::AttributeLocation::BoneWeights) |
            (1 << (int) Shader::AttributeLocation::BoneIndices);
     
        for (int i = 0; i < m_primitives.size(); ++i)
        {
            driver.destroyRenderPrimitive(m_primitives[i]);
			m_primitives[i].clear();
        }
        m_primitives.clear();
        
        m_primitives.resize(m_submeshes.size());
        for (int i = 0; i < m_primitives.size(); ++i)
        {
            m_primitives[i] = driver.createRenderPrimitive();
            
            driver.setRenderPrimitiveBuffer(m_primitives[i], m_vb, m_ib, m_enabled_attributes);
            driver.setRenderPrimitiveRange(m_primitives[i], m_primitive_type, m_submeshes[i].index_first, 0, m_vertices.size() - 1, m_submeshes[i].index_count);
        }

        RecalculateBoundsInternal();
    }

    void Mesh::SetBlendShapes(std::vector<BlendShape>&& blend_shapes)
    {
        m_blend_shapes = std::move(blend_shapes);
    }

    void Mesh::RecalculateBoundsInternal()
    {
		MinMaxAABB minmax;
		for (size_t i = 0; i < m_vertices.size(); ++i)
		{
			minmax.Encapsulate(m_vertices[i].vertex);
		}
		AABB aabb;
		if (m_vertices.size())
			aabb = AABB(minmax);
		else
			aabb = AABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));

		m_LocalAABB = aabb;
    }
}
