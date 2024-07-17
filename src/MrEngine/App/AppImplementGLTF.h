#include "AppInclude.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"

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
            //model->GetTransform()->SetPosition(Vector3(0, 0, 1));
            model->GetTransform()->SetPosition(-aabb.GetCenter());
            model->GetTransform()->SetRotation(Quaternion::Euler(0, 0, 0));
            //model->GetTransform()->SetScale(Vector3(1, 1, 1));
            std::shared_ptr<Camera> camera = m_pScene->CreateEntity("camera")->AddComponent<Camera>();
            Camera::SetMainCamera(camera);
            camera->GetTransform()->SetPosition(Vector3(0.f, 0.0f, 3.5f));
            camera->GetTransform()->SetRotation(Quaternion::Euler(m_camera_rot));
            camera->SetNearClip(0.03f);
            camera->SetFarClip(100);
            camera->SetDepth(2);
            camera->SetCullingMask((1 << 0) | (1 << 4) | (1 << 8));
            camera->SetClearColor(Color(0.22, 0.22, 0.22, 1.0));
            //camera->SetLeftHandSpace(true);

			auto animations = model->GetComponentsInChildren<Animation>();
			if (animations.size() > 0)
			{
				animations[0]->Play(0);
			}
            auto light = m_pScene->CreateEntity("light")->AddComponent<Light>();
            light->GetTransform()->SetRotation(Quaternion::Euler(m_camera_rot));
            light->SetType(LightType::Directional);
            light->SetAmbientColor(Color(0.22, 0.22, 0.22, 1.0));

            InitAudio(); 
        }

		void InitAudio()
		{
			//auto listener = AudioManager::GetListener(m_pScene);
			//listener->GetTransform()->SetLocalPosition(Vector3(0, 0, 0));
			//listener->GetTransform()->SetLocalRotation(Quaternion::Euler(0, 0, 0));

#if !VR_WASM
			auto audio_path = "audio/lightning.mp3";
			auto audio_clip = AudioClip::LoadMp3FromFile(Engine::Instance()->GetDataPath() + "/" + audio_path);
			auto audio_source = m_pScene->CreateEntity("as")->AddComponent<AudioSource>().get();
			audio_source->SetClip(audio_clip);
			audio_source->SetLoop(false);
			audio_source->Play();
#else
			auto audio_path = "audio/Unite In The Sky (full).mp3";
			AudioManager::PlayAudio(audio_path, false);
#endif
		}

        virtual ~AppImplementGLTF()
        {

        }

        void Update()
        {


        }
    };
}
