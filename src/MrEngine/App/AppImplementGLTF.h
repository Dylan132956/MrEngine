#include "AppInclude.h"

namespace moonriver
{
    class AppImplementGLTF : public AppImplement
    {
    public:
        Camera* m_camera = nullptr;
        Camera* m_reflection_camera = nullptr;
        Camera* m_back_screen_camera = nullptr;
        Material* m_reflection_material = nullptr;
        Vector2 m_last_touch_pos;
        Vector3 m_camera_rot = Vector3(5, 180, 0);
        float m_start_time = -1;
        bool m_audio_init = false;

        AppImplementGLTF(Engine* pEngine)
        {
            m_pEngine = pEngine;
            m_pScene = m_pEngine->CreateScene();
            m_pScene->LoadGLTF("/model/DamagedHelmet/DamagedHelmet.gltf");
            std::shared_ptr<Camera> camera = m_pScene->CreateEntity("camera")->AddComponent<Camera>();
            Camera::SetMainCamera(camera);
            camera->GetTransform()->SetPosition(Vector3(0, 1.0f, 3.5f));
            camera->GetTransform()->SetRotation(Quaternion::Euler(m_camera_rot));
            camera->SetNearClip(0.03f);
            camera->SetFarClip(100);
            camera->SetDepth(2);
            camera->SetCullingMask((1 << 0) | (1 << 4) | (1 << 8));
        }

        virtual ~AppImplementGLTF()
        {

        }

        void Update()
        {


        }
    };
}
