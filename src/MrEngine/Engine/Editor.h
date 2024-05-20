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

#include "math/Rect.h"

namespace moonriver
{
    class Entity;
    class ImGuiRenderer;

    class Editor
    {
    public:
        bool IsInEditorMode() const { return m_editor_mode; }
        std::shared_ptr<Entity> GetSelectedGameObject() const { return m_selected_object.lock(); }
        void Update();

    private:
        void DrawWindows();

    private:
        bool m_editor_mode = false;
        std::weak_ptr<Entity> m_selected_object;
        std::weak_ptr<ImGuiRenderer> m_imgui;
        std::vector<Rect> m_imgui_window_rects;
    };
}
