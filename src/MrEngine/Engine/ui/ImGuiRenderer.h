/*
* moonriver
* Copyright 2023-2024 by Dylan - 13227110@qq.com
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
    class Camera;
    class ImGuiRenderer : public MeshRenderer
    {
    public:
        ImGuiRenderer();
        virtual ~ImGuiRenderer();
        void UpdateImGui();
        void SetDrawAction(Action draw) { m_draw = draw; }
        std::shared_ptr<Camera> GetCamera() const { return m_camera.lock(); }
        void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }

    private:
        Action m_draw;
        std::shared_ptr<Texture> m_font_texture;
        std::weak_ptr<Camera> m_camera;
    };
}
