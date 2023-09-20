#pragma once
#include "private/backend/DriverApi.h"
#include "math/Matrix4x4.h"

namespace moonriver
{
    struct mvpUniforms
    {
        static constexpr const char* M_MATRIX = "u_WorldMatrix";
        static constexpr const char* V_MATRIX = "u_ViewMatrix";
        static constexpr const char* P_MATRIX = "u_ProjectionMatrix";
        Matrix4x4 uWorldMatrix;
        Matrix4x4 uViewMatrix;
        Matrix4x4 uProjectionMatrix;
    };

    class triangle
    {
    public:
        triangle();
        ~triangle();
        void run();
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