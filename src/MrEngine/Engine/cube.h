#pragma once
#include "private/backend/DriverApi.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"

namespace moonriver
{
    class cube
    {
    public:
        cube();
        ~cube();
        void Render();
    private:
        static constexpr size_t WIREFRAME_OFFSET = 3 * 2 * 6;
        static const uint32_t mIndices[];
        static const uint32_t mIndices_Tex[];
        static const filament::math::float3 mVertices[];

        filament::backend::AttributeArray m_attributes;
        uint32_t m_enabled_attributes;
        filament::backend::VertexBufferHandle m_vb;
        filament::backend::IndexBufferHandle m_ib;
        filament::backend::PrimitiveType m_primitive_type;
        std::vector<filament::backend::RenderPrimitiveHandle> m_primitives;
        filament::backend::PipelineState pipeline;
        filament::backend::UniformBufferHandle m_uniform_buffer;
        mvpUniforms m_uniforms;
        std::shared_ptr<Texture> cube_texture;
        filament::backend::SamplerGroupHandle m_cube_sampler_group;
    };
}