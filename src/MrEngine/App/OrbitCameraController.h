#pragma once

#include "Input.h"
#include "Engine.h"
#include "Editor.h"
#include "Transform.h"
#include "graphics/Camera.h"
#include "math/Mathf.h"
#include "math/Quaternion.h"
#include "time/Time.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <cmath>

namespace moonriver
{
    // Orbit / pan / zoom around a target, plus WASD fly. Mouse buttons: 0 = left, 1 = right, 2 = middle.
    //   Left drag          orbit
    //   Right drag         rotate the model (trackball, environment stays fixed)
    //   Middle drag        pan
    //   Wheel              zoom
    //   W/S A/D Q/E        move forward/back, left/right, down/up (Shift = faster)
    //   F                  reset view and model rotation
    class OrbitCameraController
    {
    public:
        void Init(Camera* camera, const Vector3& target, float distance, float pitch, float yaw)
        {
            m_camera = camera;
            m_defaultTarget = m_target = target;
            m_defaultDistance = m_distance = distance;
            m_defaultPitch = m_pitch = pitch;
            m_defaultYaw = m_yaw = yaw;
            Apply();
        }

        // The model rotates about its bounds center, which is placed at the world origin.
        void SetModel(const std::shared_ptr<Transform>& model, const Vector3& center)
        {
            m_model = model;
            m_modelCenter = center;
            m_modelRotation = Quaternion::Identity();
            ApplyModel();
        }

        void Update()
        {
            if (!m_camera)
            {
                return;
            }

            const Vector3 mouse = Input::GetMousePosition();
            const bool uiWantsMouse = IsUIHoveringMouse();
            const bool uiWantsKeyboard = IsUITypingKeyboard();

            for (int i = 0; i < 3; ++i)
            {
                if (Input::GetMouseButtonDown(i) && !uiWantsMouse)
                {
                    m_dragging[i] = true;
                }
                if (!Input::GetMouseButton(i))
                {
                    m_dragging[i] = false;
                }
            }

            const float dx = mouse.x - m_lastMouse.x;
            const float dy = mouse.y - m_lastMouse.y;
            m_lastMouse = mouse;

            if (m_dragging[0])
            {
                m_yaw += dx * kRotateSpeed;
                // Mouse Y is bottom-up; dragging up tilts the view to look from below.
                m_pitch = Mathf::Clamp(m_pitch + dy * kRotateSpeed, -89.0f, 89.0f);
            }
            else if (m_dragging[1])
            {
                if (m_model)
                {
                    // Turn the side facing the camera in the drag direction.
                    const Quaternion rot = Quaternion::Euler(m_pitch, m_yaw, 0.0f);
                    m_modelRotation = Quaternion::AngleAxis(dy * kRotateSpeed, rot * Vector3(1, 0, 0))
                        * Quaternion::AngleAxis(dx * kRotateSpeed, rot * Vector3(0, 1, 0))
                        * m_modelRotation;
                    ApplyModel();
                }
            }
            else if (m_dragging[2])
            {
                const Quaternion rot = Quaternion::Euler(m_pitch, m_yaw, 0.0f);
                const float panScale = m_distance * kPanSpeed;
                m_target = m_target - (rot * Vector3(1, 0, 0)) * (dx * panScale) - (rot * Vector3(0, 1, 0)) * (dy * panScale);
            }

            const float wheel = Input::GetMouseScrollWheel();
            if (wheel != 0.0f && !uiWantsMouse)
            {
                m_distance = Mathf::Clamp(m_distance * (float)std::pow(kZoomStep, wheel), kMinDistance, kMaxDistance);
            }

            const bool ctrl = Input::GetKey(KeyCode::LeftControl) || Input::GetKey(KeyCode::RightControl);
            if (!uiWantsKeyboard && !ctrl)
            {
                UpdateKeyboard();
            }

            Apply();
        }

    private:
        static constexpr float kRotateSpeed = 0.3f;
        static constexpr float kPanSpeed = 0.0015f;
        static constexpr float kZoomStep = 0.9f;
        static constexpr float kMoveSpeed = 1.0f;
        static constexpr float kMinDistance = 0.05f;
        static constexpr float kMaxDistance = 100.0f;

        void UpdateKeyboard()
        {
            if (Input::GetKeyDown(KeyCode::F))
            {
                m_target = m_defaultTarget;
                m_distance = m_defaultDistance;
                m_pitch = m_defaultPitch;
                m_yaw = m_defaultYaw;
                m_modelRotation = Quaternion::Identity();
                ApplyModel();
                return;
            }

            float forward = 0, right = 0, up = 0;
            if (Input::GetKey(KeyCode::W)) forward += 1;
            if (Input::GetKey(KeyCode::S)) forward -= 1;
            if (Input::GetKey(KeyCode::D)) right += 1;
            if (Input::GetKey(KeyCode::A)) right -= 1;
            if (Input::GetKey(KeyCode::E)) up += 1;
            if (Input::GetKey(KeyCode::Q)) up -= 1;
            if (forward == 0 && right == 0 && up == 0)
            {
                return;
            }

            const bool shift = Input::GetKey(KeyCode::LeftShift) || Input::GetKey(KeyCode::RightShift);
            // Scale with distance so moving feels the same whether zoomed in or out.
            const float step = kMoveSpeed * std::max(m_distance, 0.2f) * (shift ? 3.0f : 1.0f) * Time::GetDeltaTime();
            const Quaternion rot = Quaternion::Euler(m_pitch, m_yaw, 0.0f);
            m_target = m_target
                + (rot * Vector3(0, 0, 1)) * (forward * step)
                + (rot * Vector3(1, 0, 0)) * (right * step)
                + Vector3(0, 1, 0) * (up * step);
        }

        void Apply()
        {
            const Quaternion rot = Quaternion::Euler(m_pitch, m_yaw, 0.0f);
            auto transform = m_camera->GetTransform();
            transform->SetRotation(rot);
            transform->SetPosition(m_target - (rot * Vector3(0, 0, 1)) * m_distance);
        }

        void ApplyModel()
        {
            if (!m_model)
            {
                return;
            }
            m_model->SetRotation(m_modelRotation);
            m_model->SetPosition(m_modelRotation * (Vector3(0, 0, 0) - m_modelCenter));
        }

        static bool IsEditorUIActive()
        {
            Engine* engine = Engine::Instance();
            return engine && engine->GetEditor() && engine->GetEditor()->IsInEditorMode() && ImGui::GetCurrentContext();
        }

        static bool IsUIHoveringMouse()
        {
            return IsEditorUIActive() && ImGui::GetIO().WantCaptureMouse;
        }

        static bool IsUITypingKeyboard()
        {
            return IsEditorUIActive() && ImGui::GetIO().WantCaptureKeyboard;
        }

        Camera* m_camera = nullptr;
        Vector3 m_target;
        float m_distance = 3.5f;
        float m_pitch = 0.0f;
        float m_yaw = 180.0f;
        Vector3 m_defaultTarget;
        float m_defaultDistance = 3.5f;
        float m_defaultPitch = 0.0f;
        float m_defaultYaw = 180.0f;
        Vector3 m_lastMouse;
        bool m_dragging[3] = { false, false, false };
        std::shared_ptr<Transform> m_model;
        Vector3 m_modelCenter;
        Quaternion m_modelRotation = Quaternion::Identity();
    };
}
