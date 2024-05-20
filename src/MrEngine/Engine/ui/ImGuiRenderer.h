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

#include "graphics/MeshRenderer.h"
#include "Action.h"

namespace moonriver
{
    class ImGuiRenderer : public MeshRenderer
    {
    public:
        ImGuiRenderer();
        virtual ~ImGuiRenderer();
        void UpdateImGui();
        void SetDrawAction(Action draw) { m_draw = draw; }
        Ref<Camera> GetCamera() const { return m_camera.lock(); }
        void SetCamera(const Ref<Camera>& camera) { m_camera = camera; }

    private:
        Action m_draw;
        Ref<Texture> m_font_texture;
        WeakRef<Camera> m_camera;
    };
}
