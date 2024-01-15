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
        Vector3 m_camera_rot = Vector3(0, 180, 0);
        float m_start_time = -1;
        bool m_audio_init = false;

        AppImplementGLTF(Engine* pEngine)
        {
            m_pEngine = pEngine;
            m_pScene = m_pEngine->CreateScene();
            ///model/Sponza/Sponza.gltf
            //m_pScene->LoadGLTF("/model/DamagedHelmet/DamagedHelmet.gltf");
            ///model/BrainStem/glTF/BrainStem.gltf

            auto model = m_pScene->LoadGLTF("/model/BrainStem/glTF/BrainStem.gltf");
            AABB aabb = model->RecalculateBoundsInternal();

            //model->GetTransform()->SetPosition(Vector3(0, 0, 0));
            model->GetTransform()->SetPosition(-aabb.GetCenter());
            model->GetTransform()->SetRotation(Quaternion::Euler(0, 0, 0));
            std::shared_ptr<Camera> camera = m_pScene->CreateEntity("camera")->AddComponent<Camera>();
            Camera::SetMainCamera(camera);
            camera->GetTransform()->SetPosition(Vector3(0.f, 0.0f, 3.5f));
            camera->GetTransform()->SetRotation(Quaternion::Euler(m_camera_rot));
            camera->SetNearClip(0.03f);
            camera->SetFarClip(100);
            camera->SetDepth(2);
            camera->SetCullingMask((1 << 0) | (1 << 4) | (1 << 8));
            camera->SetClearColor(Color(0.22, 0.22, 0.22, 1.0));

			auto animations = model->GetComponentsInChildren<Animation>();
			if (animations.size() > 0)
			{
				animations[0]->Play(0);
			}

            //camera->SetLeftHandSpace(false);

            auto light = m_pScene->CreateEntity("light")->AddComponent<Light>();
            light->GetTransform()->SetRotation(Quaternion::Euler(60, 90, 0));
            light->SetType(LightType::Directional);
        }

        virtual ~AppImplementGLTF()
        {

        }

        void Update()
        {


        }
    };
}
