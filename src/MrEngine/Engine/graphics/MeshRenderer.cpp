#include "MeshRenderer.h"

namespace moonriver
{
    MeshRenderer::MeshRenderer()
    {

    }
    
    MeshRenderer::~MeshRenderer()
    {

    }
    
    void MeshRenderer::SetMesh(const std::shared_ptr<Mesh>& mesh)
    {
        m_mesh = mesh;
    }
    
    std::vector<filament::backend::RenderPrimitiveHandle> MeshRenderer::GetPrimitives()
    {
        std::vector<filament::backend::RenderPrimitiveHandle> primitives;
        
        if (m_mesh)
        {
            primitives = m_mesh->GetPrimitives();
        }
        
        return primitives;
    }

    Bounds MeshRenderer::GetLocalBounds() const
    {
        Bounds bounds;

        if (m_mesh)
        {
            bounds = m_mesh->GetBounds();
        }

        return bounds;
    }

    AABB MeshRenderer::GetLocalAABB() const
    {
        AABB aabb;
		if (m_mesh)
		{
            aabb = m_mesh->GetAABB();
		}
        else
        {
            int k = 3;
        }

		return aabb;
    }
}
