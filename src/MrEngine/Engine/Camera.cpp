#include "Camera.h"
#include "Engine.h"

namespace moonriver
{
    Camera::Camera()
    {

    }
    Camera::~Camera()
    {

    }
    //const Matrix4x4& Camera::GetViewMatrix()
    //{
    //    if (m_view_matrix_dirty)
    //    {
    //        m_view_matrix_dirty = false;

    //        if (!m_view_matrix_external)
    //        {
    //            m_view_matrix = Matrix4x4::LookTo(this->GetTransform()->GetPosition(), this->GetTransform()->GetForward(), this->GetTransform()->GetUp());
    //        }
    //    }

    //    return m_view_matrix;
    //}

    //const Matrix4x4& Camera::GetProjectionMatrix()
    //{
    //    if (m_projection_matrix_dirty)
    //    {
    //        m_projection_matrix_dirty = false;

    //        if (!m_projection_matrix_external)
    //        {
    //            float view_width = this->GetTargetWidth() * m_viewport_rect.w;
    //            float view_height = this->GetTargetHeight() * m_viewport_rect.h;
    //            float aspect = m_aspect;
    //            if (aspect <= 0)
    //            {
    //                aspect = view_width / view_height;
    //            }

    //            if (m_orthographic)
    //            {
    //                float ortho_size = m_orthographic_size;
    //                float top = ortho_size;
    //                float bottom = -ortho_size;
    //                float plane_h = ortho_size * 2;
    //                float plane_w = plane_h * aspect;
    //                m_projection_matrix = Matrix4x4::Ortho(-plane_w / 2, plane_w / 2, bottom, top, m_near_clip, m_far_clip);
    //            }
    //            else
    //            {
    //                m_projection_matrix = Matrix4x4::Perspective(m_field_of_view, aspect, m_near_clip, m_far_clip);
    //            }
    //        }
    //    }

    //    return m_projection_matrix;
    //}
    //int Camera::GetTargetWidth() const
    //{
    //    if (m_render_target_color)
    //    {
    //        return m_render_target_color->GetWidth();
    //    }
    //    else if (m_render_target_depth)
    //    {
    //        return m_render_target_depth->GetWidth();
    //    }
    //    else
    //    {
    //        return Engine::Instance()->GetWidth();
    //    }
    //}

    //int Camera::GetTargetHeight() const
    //{
    //    if (m_render_target_color)
    //    {
    //        return m_render_target_color->GetHeight();
    //    }
    //    else if (m_render_target_depth)
    //    {
    //        return m_render_target_depth->GetHeight();
    //    }
    //    else
    //    {
    //        return Engine::Instance()->GetHeight();
    //    }
    //}
}
