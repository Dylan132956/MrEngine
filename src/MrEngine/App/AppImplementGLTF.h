#include "AppInclude.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioListener.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"
#include "graphics/Mesh.h"
#include "graphics/Material.h"
#include "graphics/Shader.h"
#include "graphics/PBREnvironment.h"

namespace moonriver
{
    class AppImplementGLTF : public AppImplement
    {
    public:
        Camera* m_camera = nullptr;
        Vector2 m_last_touch_pos;
        Vector3 m_camera_rot = Vector3(5, 180, 0);
        GLTFAsyncLoadHandle m_gltf_async_handle;
        std::shared_ptr<Entity> m_model;
        std::shared_ptr<Entity> m_skybox;
        bool m_gltf_async_done = false;

        static std::shared_ptr<Mesh> CreateSkyboxCubeMesh()
        {
            // Unit cube; skybox shader culls front faces so we see the interior.
            const float s = 1.0f;
            auto makeV = [](float x, float y, float z) {
                Mesh::Vertex v = {};
                v.vertex = Vector4(x, y, z, 1);
                v.color = Color(1, 1, 1, 1);
                v.normal = Vector3(x, y, z);
                return v;
            };

            std::vector<Mesh::Vertex> vertices = {
                makeV(-s, -s, -s), makeV(s, -s, -s), makeV(s, s, -s), makeV(-s, s, -s),
                makeV(-s, -s,  s), makeV(s, -s,  s), makeV(s, s,  s), makeV(-s, s,  s),
            };
            std::vector<unsigned int> indices = {
                0,1,2, 0,2,3,
                4,6,5, 4,7,6,
                0,3,7, 0,7,4,
                1,5,6, 1,6,2,
                3,2,6, 3,6,7,
                0,4,5, 0,5,1,
            };
            return std::make_shared<Mesh>(std::move(vertices), std::move(indices));
        }

        void CreateSkybox()
        {
            auto& ibl = PBREnvironment::Instance();
            m_skybox = m_pScene->CreateEntity("skybox");
            auto renderer = m_skybox->AddComponent<MeshRenderer>();
            renderer->SetMesh(CreateSkyboxCubeMesh());
            auto mat = std::make_shared<Material>(Shader::Find("skybox"));
            mat->SetTexture("SPIRV_Cross_Combinedg_SkyboxMapg_SkyboxSampler", ibl.GetPrefilteredMap());
            mat->SetQueue((int)Shader::Queue::Background);
            renderer->SetMaterial(mat);
            m_skybox->SetLayer(0);
        }

        AppImplementGLTF(Engine* pEngine)
        {
            m_pEngine = pEngine;
            m_pScene = m_pEngine->CreateScene();

            // Prefer a real HDRI (Poly Haven studio); fall back to high-contrast procedural.
            {
                std::string dataPath = Engine::Instance()->GetDataPath();
                // Same environment as DiligentSamples GLTFViewer. Diligent mirrors glTF Y (InvYAxis), so the
                // cubemap is Y-flipped relative to the model there; papermill_mrengine.ktx bakes in that flip.
                const std::string orientedKtx = dataPath + "/env/papermill_mrengine.ktx";
                bool hdrOk = PBREnvironment::Instance().InitializeFromKTX(orientedKtx);
                if (!hdrOk)
                {
                    hdrOk = PBREnvironment::Instance().InitializeFromKTXReoriented(
                        dataPath + "/env/papermill.ktx", 1.0f, -1.0f, 1.0f, orientedKtx);
                }
                const char* hdrCandidates[] = {
                    "/env/photo_studio_loft_hall_1k.hdr",
                    "/env/brown_photostudio_02_1k.hdr",
                    "/env/studio_small_03_1k.hdr",
                };
                for (const char* rel : hdrCandidates)
                {
                    if (hdrOk)
                    {
                        break;
                    }
                    if (PBREnvironment::Instance().InitializeFromHDR(dataPath + rel))
                    {
                        hdrOk = true;
                        break;
                    }
                }
                if (!hdrOk)
                {
                    PBREnvironment::Instance().InitializeProcedural();
                }
            }

            // DamagedHelmet is the DiligentFX README reference model.
            // Sponza: m_pScene->LoadGLTFAsync("/model/Sponza/Sponza.gltf");
            m_gltf_async_handle = m_pScene->LoadGLTFAsync("/model/DamagedHelmet/DamagedHelmet.gltf");

            std::shared_ptr<Camera> camera = m_pScene->CreateEntity("camera")->AddComponent<Camera>();
            Camera::SetMainCamera(camera);
            m_camera = camera.get();
            camera->GetTransform()->SetPosition(Vector3(0.f, 0.0f, 3.5f));
            camera->GetTransform()->SetRotation(Quaternion::Euler(m_camera_rot));
            camera->SetNearClip(0.03f);
            camera->SetFarClip(100);
            camera->SetDepth(2);
            camera->SetCullingMask((1 << 0) | (1 << 4) | (1 << 8));
            camera->SetClearColor(Color(0.02f, 0.02f, 0.03f, 1.0f));

            auto light = m_pScene->CreateEntity("light")->AddComponent<Light>();
            light->GetTransform()->SetRotation(Quaternion::Euler(35.0f, 180.0f, 0.0f));
            light->SetType(LightType::Directional);
            light->SetColor(Color(1.0f, 0.98f, 0.92f, 1.0f));
            light->SetIntensity(3.0f);
            // IBL provides indirect lighting; keep ambient off to avoid wash-out.
            Light::SetAmbientColor(Color(0.0f, 0.0f, 0.0f, 1.0f));

            CreateSkybox();
            InitAudio();
        }

        void InitAudio()
        {
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

                        // Materials are created during async assemble; bind IBL now.
                        PBREnvironment::Instance().ApplyToSceneMaterials();
                    }
                }
                else
                {
                    printf("LoadGLTFAsync failed: %s\n", m_gltf_async_handle.GetError().c_str());
                }
            }
        }
    };
}
