#pragma once

#include "Object.h"
#include "Color.h"
#include "math/Vector2.h"
#include "math/Matrix4x4.h"
#include "math/Bounds.h"
#include "private/backend/DriverApi.h"

namespace moonriver
{
    class Texture;

    class Mesh : public Object
    {
    public:
        struct Vertex
        {
            Vector4 vertex;
            Color color;
            Vector2 uv;
            Vector2 uv2;
            Vector3 normal;
            Vector4 tangent;
            Vector4 bone_weights;
            Vector4 bone_indices;
        };
        
        struct Submesh
        {
            unsigned int index_first;
            unsigned int index_count;
        };
        
        struct BlendShapeFrame
        {
            std::vector<Vector3> vertices;
            std::vector<Vector3> normals;
            std::vector<Vector3> tangents;
        };
        
        struct BlendShape
        {
            std::string name;
            BlendShapeFrame frame;
        };

    public:
		static void Init();
		static void Done();
		static const std::shared_ptr<Mesh>& GetSharedQuadMesh();
        static const std::shared_ptr<Mesh>& GetSharedBoundsMesh();
        static std::shared_ptr<Mesh> LoadFromFile(const std::string& path);
        Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, const std::vector<Submesh>& submeshes = std::vector<Submesh>(), bool uint32_index = false, bool dynamic = false, filament::backend::PrimitiveType primitive_type = filament::backend::PrimitiveType::TRIANGLES);
        virtual ~Mesh();
        void Update(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, const std::vector<Submesh>& submeshes = std::vector<Submesh>());
        const std::vector<Vertex>& GetVertices() const { return m_vertices; }
        const std::vector<unsigned int>& GetIndices() const { return m_indices; }
        const std::vector<Submesh>& GetSubmeshes() const { return m_submeshes; }
        const std::vector<Matrix4x4>& GetBindposes() const { return m_bindposes; }
        const std::vector<BlendShape>& GetBlendShapes() const { return m_blend_shapes; }
        const std::shared_ptr<Texture>& GetBlendShapeTexture() const { return m_blend_shape_texture; }
        const Bounds& GetBounds() const { return m_bounds; }
		const AABB& GetAABB() const {
            return m_LocalAABB;
		}
		const filament::backend::AttributeArray& GetAttributes() const { return m_attributes; }
		uint32_t GetEnabledAttributes() const { return m_enabled_attributes; }
		const filament::backend::VertexBufferHandle& GetVertexBuffer() const { return m_vb; }
		const filament::backend::IndexBufferHandle& GetIndexBuffer() const { return m_ib; }
		const std::vector<filament::backend::RenderPrimitiveHandle>& GetPrimitives() const { return m_primitives; }
        void SetBindposes(std::vector<Matrix4x4>&& bindposes) { m_bindposes = std::move(bindposes); }
        void RecalculateBoundsInternal();
    private:
        void SetBlendShapes(std::vector<BlendShape>&& blend_shapes);
    private:
		static std::shared_ptr<Mesh> m_shared_quad_mesh;
        static std::shared_ptr<Mesh> m_shared_bounds_mesh;
        std::vector<Vertex> m_vertices;
        std::vector<unsigned int> m_indices;
        int m_buffer_vertex_count;
        int m_buffer_index_count;
        std::vector<Submesh> m_submeshes;
        std::vector<Matrix4x4> m_bindposes;
        std::vector<BlendShape> m_blend_shapes;
        std::shared_ptr<Texture> m_blend_shape_texture;
        Bounds m_bounds;
        AABB m_LocalAABB;
        bool m_uint32_index;
		filament::backend::AttributeArray m_attributes;
		uint32_t m_enabled_attributes;
        filament::backend::VertexBufferHandle m_vb;
        filament::backend::IndexBufferHandle m_ib;
        filament::backend::PrimitiveType m_primitive_type;
        std::vector<filament::backend::RenderPrimitiveHandle> m_primitives;
    };
}
