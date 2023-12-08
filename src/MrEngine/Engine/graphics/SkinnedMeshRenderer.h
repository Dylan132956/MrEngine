#pragma once

#include "MeshRenderer.h"

namespace moonriver
{
    class SkinnedMeshRenderer : public MeshRenderer
    {
    public:
        SkinnedMeshRenderer();
        virtual ~SkinnedMeshRenderer();
		virtual void SetMesh(const std::shared_ptr<Mesh>& mesh);
        const std::vector<std::string>& GetBonePaths() const { return m_bone_paths; }
        void SetBonePaths(const std::vector<std::string>& bones) { m_bone_paths = bones; }
        std::shared_ptr<Transform> GetBonesRoot() const { return m_bones_root.lock(); }
        void SetBonesRoot(const std::shared_ptr<Transform>& node) { m_bones_root = node; }
        void SetBones(const std::vector<std::shared_ptr<Transform>>& bones);
        float GetBlendShapeWeight(const std::string& name);
        void SetBlendShapeWeight(const std::string& name, float weight);
		const std::vector<Vector4>& GetBoneVectors() const { return m_bone_vectors; };
        const filament::backend::UniformBufferHandle& GetBonesUniformBuffer() const { return m_bones_uniform_buffer; }
        const filament::backend::SamplerGroupHandle& GetBlendShapeSamplerGroup() const { return m_blend_shape_sampler_group; }
        virtual std::vector<filament::backend::RenderPrimitiveHandle> GetPrimitives();
        
	protected:
		virtual void Prepare();

    private:
        void FindBones();

	private:
		struct BlendShapeWeight
		{
			int index;
			float weight;
		};

    private:
        std::vector<std::string> m_bone_paths;
        std::weak_ptr<Transform> m_bones_root;
        std::vector<std::weak_ptr<Transform>> m_bones;
		std::map<std::string, BlendShapeWeight> m_blend_shape_weights;
		bool m_blend_shape_dirty;
		std::vector<Vector4> m_bone_vectors;
        filament::backend::UniformBufferHandle m_bones_uniform_buffer;
        filament::backend::SamplerGroupHandle m_blend_shape_sampler_group;
		filament::backend::VertexBufferHandle m_vb;
		std::vector<filament::backend::RenderPrimitiveHandle> m_primitives;
		int m_vb_vertex_count;
		std::vector<Mesh::Submesh> m_submeshes;
    };
}
