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
        GLTFAsyncLoadHandle m_gltf_async_handle;
        ImagePipeline::ImageSequenceLoadHandle m_sequenceHandle;
        bool m_sequence_async_done = false;
        std::shared_ptr<Entity> m_model;
        bool m_gltf_async_done = false;

        AppImplementGLTF(Engine* pEngine)
        {
            m_pEngine = pEngine;
            m_pScene = m_pEngine->CreateScene();
            ///model/Sponza/Sponza.gltf
            //m_pScene->LoadGLTF("/model/DamagedHelmet/DamagedHelmet.gltf");
            ///model/BrainStem/glTF/BrainStem.gltf

            //auto model = m_pScene->LoadGLTF("/model/BrainStem/glTF/BrainStem.gltf");
            //auto model = m_pScene->LoadGLTF("/model/Sponza/Sponza.gltf");
            m_gltf_async_handle = m_pScene->LoadGLTFAsync("/model/Sponza/Sponza.gltf");
            // async glTF result is handled in Update().

            // Image sequence async loading example. Replace paths with existing frame images under data path.
			std::vector<std::string> sequenceFrames;
			sequenceFrames.push_back("/effect/sequence/01_00000.png");
			sequenceFrames.push_back("/effect/sequence/01_00001.png");
			sequenceFrames.push_back("/effect/sequence/01_00002.png");
			sequenceFrames.push_back("/effect/sequence/01_00003.png");
			sequenceFrames.push_back("/effect/sequence/01_00004.png");
			sequenceFrames.push_back("/effect/sequence/01_00005.png");
			sequenceFrames.push_back("/effect/sequence/01_00006.png");
			sequenceFrames.push_back("/effect/sequence/01_00007.png");
			sequenceFrames.push_back("/effect/sequence/01_00008.png");
			sequenceFrames.push_back("/effect/sequence/01_00009.png");
			sequenceFrames.push_back("/effect/sequence/01_00010.png");
			sequenceFrames.push_back("/effect/sequence/01_00011.png");
			sequenceFrames.push_back("/effect/sequence/01_00012.png");
			sequenceFrames.push_back("/effect/sequence/01_00013.png");
			sequenceFrames.push_back("/effect/sequence/01_00014.png");
			sequenceFrames.push_back("/effect/sequence/01_00015.png");
			sequenceFrames.push_back("/effect/sequence/01_00016.png");
			sequenceFrames.push_back("/effect/sequence/01_00017.png");
			sequenceFrames.push_back("/effect/sequence/01_00018.png");
            m_sequenceHandle = m_pScene->LoadImageSequenceAsync("example_sequence", sequenceFrames, 24.0f, true);
            // Use sequenceHandle.IsDone(), sequenceHandle.IsReady(), sequenceHandle.GetAsset(),
            // and ImageSequenceAsset::Sample(timeSeconds) to fetch the current frame texture.
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
            if (!m_gltf_async_done && m_gltf_async_handle.IsDone())
            {
                m_gltf_async_done = true;
                if (m_gltf_async_handle.IsReady())
                {
                    m_model = m_gltf_async_handle.GetRoot();
                    if (m_model)
                    {
                        AABB aabb = m_model->RecalculateBoundsInternal();
                        m_model->GetTransform()->SetPosition(-aabb.GetCenter());
                        m_model->GetTransform()->SetRotation(Quaternion::Euler(0, 0, 0));

                        auto animations = m_model->GetComponentsInChildren<Animation>();
                        if (animations.size() > 0)
                        {
                            animations[0]->Play(0);
                        }
                    }
                }
                else
                {
                    printf("LoadGLTFAsync failed: %s\n", m_gltf_async_handle.GetError().c_str());
                }
            }
            if (!m_sequence_async_done && m_sequenceHandle.IsDone())
            {
                m_sequence_async_done = true;
                if (m_sequenceHandle.IsReady())
                {
                    auto res = m_sequenceHandle.GetAsset();
                    auto tex = res->Sample(0.5);
                    auto tex1 = res->GetFrame(0);
                    auto count = res->GetFrameCount();
             
					// Use sequenceHandle.IsDone(), sequenceHandle.IsReady(), sequenceHandle.GetAsset(),
// and ImageSequenceAsset::Sample(timeSeconds) to fetch the current frame texture.
                }
            }
        }
    };
}
