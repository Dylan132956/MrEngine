/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "Component.h"
#include "Material.h"
#include "math/Vector4.h"
#include "math/Bounds.h"
#include "private/backend/DriverApi.h"
#include <list>

namespace moonriver
{
    class Mesh;

    class Renderer : public Component
    {
    public:
        static const std::list<Renderer*>& GetRenderers() { return m_renderers; }
		static void PrepareAll();
        Renderer();
        virtual ~Renderer();
        std::shared_ptr<Material> GetMaterial() const;
        void SetMaterial(const std::shared_ptr<Material>& material);
        const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_materials; }
        void SetMaterials(const std::vector<std::shared_ptr<Material>>& materials);
		bool IsCastShadow() const { return m_cast_shadow; }
		void EnableCastShadow(bool enable);
		bool IsRecieveShadow() const { return m_recieve_shadow; }
		void EnableRecieveShadow(bool enable);
        int GetLightmapIndex() const { return m_lightmap_index; }
        void SetLightmapIndex(int index);
        const Vector4& GetLightmapScaleOffset() const { return m_lightmap_scale_offset; }
        void SetLightmapScaleOffset(const Vector4& vec);
        void SetShaderKeywords(const std::vector<std::string>& keywords);
        void EnableShaderKeyword(const std::string& keyword);
        const std::string& GetShaderKey(int material_index) const;
        const std::vector<std::string>& GetShaderKeywords() const;
        const RendererUniforms& GetRendererUniforms() const { return m_renderer_uniforms; }
        const filament::backend::UniformBufferHandle& GetTransformUniformBuffer() const { return m_transform_uniform_buffer; }
        virtual std::vector<filament::backend::RenderPrimitiveHandle> GetPrimitives();
        virtual Bounds GetLocalBounds() const { return Bounds(); }
        virtual AABB GetLocalAABB() const { return AABB(); }

	protected:
		virtual void Prepare();
		virtual void OnResize(int width, int height) { }

	private:
		friend class Camera;
        void UpdateShaderKeywords();

	private:
        static list<Renderer*> m_renderers;
        std::vector<std::shared_ptr<Material>> m_materials;
		bool m_cast_shadow;
		bool m_recieve_shadow;
        Vector4 m_lightmap_scale_offset;
        int m_lightmap_index;
        std::vector<std::string> m_shader_keywords;
        std::vector<std::string> m_shader_keys;
        RendererUniforms m_renderer_uniforms;
		filament::backend::UniformBufferHandle m_transform_uniform_buffer;
    };
}
