#pragma once

#include "Renderer.h"
#include "Mesh.h"

namespace moonriver
{
    class Mesh;
    
    class MeshRenderer : public Renderer
    {
    public:
        MeshRenderer();
        virtual ~MeshRenderer();
        const std::shared_ptr<Mesh>& GetMesh() const { return m_mesh; }
		virtual void SetMesh(const Ref<Mesh>& mesh);
        virtual std::vector<filament::backend::RenderPrimitiveHandle> GetPrimitives();
        virtual Bounds GetLocalBounds() const;
        virtual AABB GetLocalAABB() const;

	private:
        std::shared_ptr<Mesh> m_mesh;
    };
}
