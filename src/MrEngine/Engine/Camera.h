#pragma once

#include "math/Matrix4x4.h"
#include "math/Rect.h"

namespace moonriver
{
    class Camera
    {
    public:
        Camera();
        ~Camera();
        const Matrix4x4& GetViewMatrix();
        const Matrix4x4& GetProjectionMatrix();
        int GetTargetWidth() const;
        int GetTargetHeight() const;
    private:
        Matrix4x4 m_view_matrix;
        bool m_view_matrix_dirty;
        Matrix4x4 m_projection_matrix;
        bool m_projection_matrix_dirty;
        bool m_view_matrix_external;
        bool m_projection_matrix_external;
        Rect m_viewport_rect;
        float m_aspect;
    };
}


