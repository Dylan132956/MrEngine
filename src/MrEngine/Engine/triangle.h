#pragma once
#include "private/backend/DriverApi.h"
#include "graphics/Shader.h"

namespace moonriver
{
    class triangle
    {
    public:
        triangle();
        ~triangle();
        void Render();
    private:
        filament::backend::AttributeArray m_attributes;
        uint32_t m_enabled_attributes;
        filament::backend::VertexBufferHandle m_vb;
        filament::backend::IndexBufferHandle m_ib;
        filament::backend::PrimitiveType m_primitive_type;
        std::vector<filament::backend::RenderPrimitiveHandle> m_primitives;
        filament::backend::PipelineState pipeline;
        filament::backend::UniformBufferHandle m_uniform_buffer;
        mvpUniforms m_uniforms;
    };
}