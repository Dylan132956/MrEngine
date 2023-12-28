#pragma once

#include "Component.h"
#include "CameraClearFlags.h"
#include "Color.h"
#include "Material.h"
#include "math/Rect.h"
#include "math/Matrix4x4.h"
//#include "container/List.h"
#include "private/backend/DriverApi.h"

namespace moonriver
{
	class Texture;
    class Renderer;
	class RenderTarget;
	class Mesh;

    class Camera : public Component
    {
    public:
		static void Init();
		static void Done();
        static std::shared_ptr<Camera> GetMainCamera() { return m_main_camera.lock(); }
        static void SetMainCamera(const std::shared_ptr<Camera>& camera) { m_main_camera = camera; }
		static void RenderAll();
        static void OnResizeAll(int width, int height);
		static void Blit(const std::shared_ptr<RenderTarget>& src, const std::shared_ptr<RenderTarget>& dst, const std::shared_ptr<Material>& mat = std::shared_ptr<Material>(), int pass = -1);
		Camera();
        virtual ~Camera();
		int GetDepth() const { return m_depth; }
		void SetDepth(int depth);
        uint32_t GetCullingMask() const { return m_culling_mask; }
        void SetCullingMask(uint32_t mask);
		CameraClearFlags GetClearFlags() const { return m_clear_flags; }
		void SetClearFlags(CameraClearFlags flags);
		const Color& GetClearColor() const { return m_clear_color; }
		void SetClearColor(const Color& color);
		const Rect& GetViewportRect() const { return m_viewport_rect; }
		void SetViewportRect(const Rect& rect);
		float GetFieldOfView() const { return m_field_of_view; }
		void SetFieldOfView(float fov);
        float GetAspect() const;
        void SetAspect(float aspect);
		float GetNearClip() const { return m_near_clip; }
		void SetNearClip(float clip);
		float GetFarClip() const { return m_far_clip; }
		void SetFarClip(float clip);
		bool IsOrthographic() const { return m_orthographic; }
		void SetOrthographic(bool enable);
		float GetOrthographicSize() const { return m_orthographic_size; }
		void SetOrthographicSize(float size);
		const Matrix4x4& GetViewMatrix();
		const Matrix4x4& GetProjectionMatrix();
		void SetViewMatrixExternal(const Matrix4x4& mat);
		void SetProjectionMatrixExternal(const Matrix4x4& mat);
		const std::shared_ptr<Texture>& GetRenderTargetColor() const { return m_render_target_color; }
        const std::shared_ptr<Texture>& GetRenderTargetDepth() const { return m_render_target_depth; }
		void SetRenderTarget(const std::shared_ptr<Texture>& color, const std::shared_ptr<Texture>& depth);
		int GetTargetWidth() const;
		int GetTargetHeight() const;
        Vector3 ScreenToViewportPoint(const Vector3& position);
        Vector3 ViewportToScreenPoint(const Vector3& position);
        Vector3 ScreenToWorldPoint(const Vector3& position);
        Vector3 WorldToScreenPoint(const Vector3& position);
        Ray ScreenPointToRay(const Vector3& position);
        void SetLeftHandSpace(bool enable);
	protected:
		virtual void OnTransformDirty();

	private:
        void OnResize(int width, int height);
        void CullRenderers(const std::list<Renderer*>& renderers, std::list<Renderer*>& result);
		void UpdateViewUniforms();
		void Draw(const std::list<Renderer*>& renderers);
        void DrawRenderer(Renderer* renderer);
        void DoDraw(Renderer* renderer, bool shadow_enable = false, bool light_add = false);
        void DrawRendererBounds(Renderer* renderer);
		bool HasPostProcessing();
		void DoPostProcessing();

	private:
		static std::list<Camera*> m_cameras;
        static std::weak_ptr<Camera> m_main_camera;
		static Camera* m_current_camera;
		static bool m_cameras_order_dirty;
		static std::shared_ptr<Mesh> m_quad_mesh;
        static std::shared_ptr <Material> m_blit_material;
		int m_depth;
        uint32_t m_culling_mask;
		CameraClearFlags m_clear_flags;
		Color m_clear_color;
		Rect m_viewport_rect;
		float m_field_of_view;
        float m_aspect;
		float m_near_clip;
		float m_far_clip;
		bool m_orthographic;
		float m_orthographic_size;
		Matrix4x4 m_view_matrix;
		bool m_view_matrix_dirty;
		Matrix4x4 m_projection_matrix;
		bool m_projection_matrix_dirty;
		bool m_view_matrix_external;
		bool m_projection_matrix_external;
        bool m_leftHandSpace = false;
		std::shared_ptr<Texture> m_render_target_color;
        std::shared_ptr<Texture> m_render_target_depth;
        std::shared_ptr<RenderTarget> m_post_processing_target;
		ViewUniforms m_view_uniforms;
		filament::backend::UniformBufferHandle m_view_uniform_buffer;
		filament::backend::RenderTargetHandle m_render_target;
    };
}
