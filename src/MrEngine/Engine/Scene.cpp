
#include "Scene.h"
#include "Transform.h"
#include "Engine.h"
#include "gltfasync_loader.h"
#include "sceneasync_instance.h"
#include "asset/image/ImageDecodeLoader.h"
#include "asset/image/ImageSequence.h"

namespace moonriver
{
	//Scene* Scene::m_instance = nullptr;

	//Scene* Scene::Instance()
	//{
	//	return m_instance;
	//}

    Scene::Scene()
    {
		//m_instance = this;
        m_sceneAssetManager.RegisterLoader(std::unique_ptr<AsyncLoad::IAssetLoader>(new GLTFAsyncCpuLoader()));
        m_sceneAssetManager.RegisterLoader(std::unique_ptr<AsyncLoad::IAssetLoader>(new ImageDecode::ImageDecodeLoader()));
        m_sceneAssetManager.RegisterLoader(std::unique_ptr<AsyncLoad::IAssetLoader>(new ImagePipeline::ImageSequenceCpuLoader()));
    }
    
    Scene::~Scene()
    {
		m_added_objects.clear();
		m_removed_objects.clear();
		m_objects.clear();
		//m_instance = nullptr;
    }

    std::shared_ptr<Entity> Scene::CreateEntity(const char* name)
    {
        std::shared_ptr<Entity> obj = std::shared_ptr<Entity>(new Entity(name));
        obj->m_transform = obj->AddComponent<Transform>();
        AddEntity(obj);
        return obj;
    }

	void Scene::AddEntity(const std::shared_ptr<Entity>& obj)
	{
        obj->m_Scene = shared_from_this();
		m_added_objects.push_back(obj);
	}

	void Scene::RemoveEntity(const std::shared_ptr<Entity>& obj)
	{
        for (size_t i = 0; i < m_added_objects.size(); ++i)
        {
            if (obj == m_added_objects[i]) {
                m_added_objects.erase(m_added_objects.begin() + i);
                m_removed_objects.push_back(obj);
                break;
            }
        }
	}
    
    void Scene::Update()
    {
		for (auto& i : m_objects)
		{
			auto& obj = i.second;
			if (obj->IsActiveInTree())
			{
				obj->Update();
			}
		}

		do
		{
			std::vector<std::shared_ptr<Entity>> added = m_added_objects;
			m_added_objects.clear();

			for (int i = 0; i < added.size(); ++i)
			{
				auto& obj = added[i];
				if (obj->IsActiveInTree())
				{
					obj->Update();
				}
				m_objects[obj->GetId()] = obj;
			}
			added.clear();
		} while (m_added_objects.size() > 0);

		for (int i = 0; i < m_removed_objects.size(); ++i)
		{
			const auto& obj = m_removed_objects[i];
			m_objects.erase(obj->GetId());
		}
		m_removed_objects.clear();
        
        for (auto& i : m_objects)
        {
            auto& obj = i.second;
            if (obj->IsActiveInTree())
            {
                obj->LateUpdate();
            }
        }
    }

    void Scene::Render()
    {
        DispatchAsyncSceneLoads();
        DispatchImageSequenceAsyncLoads();
    }

    SceneAsyncLoadHandle Scene::LoadSceneAsync(const char* name)
    {
        std::string fullPath = Engine::Instance()->GetDataPath() + name;
        std::shared_ptr<SceneAsyncLoadInstance> instance(new SceneAsyncLoadInstance(this, m_sceneAssetManager, m_sceneUploadScheduler, fullPath));
        instance->Start();
        m_sceneAsyncLoads.push_back(instance);
        return SceneAsyncLoadHandle(instance);
    }
    
    GLTFAsyncLoadHandle Scene::LoadGLTFAsync(const char* name)
    {
        return LoadSceneAsync(name);
    }

    ImagePipeline::ImageSequenceLoadHandle Scene::LoadImageSequenceAsync(const char* name,
                                                                         const std::vector<std::string>& frameRelativePaths,
                                                                         float fps,
                                                                         bool loop)
    {
        ImagePipeline::ImageSequenceDesc desc;
        desc.name = name ? name : "image_sequence";
        desc.fps = fps;
        desc.loop = loop;
        desc.framePaths.reserve(frameRelativePaths.size());
        const std::string dataPath = Engine::Instance()->GetDataPath();
        for (std::size_t i = 0; i < frameRelativePaths.size(); ++i) {
            desc.framePaths.push_back(dataPath + frameRelativePaths[i]);
        }

        std::shared_ptr<ImagePipeline::ImageSequenceLoadInstance> instance(new ImagePipeline::ImageSequenceLoadInstance(
            m_sceneAssetManager,
            m_sceneUploadScheduler,
            desc));
        instance->Start();
        m_imageSequenceAsyncLoads.push_back(instance);
        return ImagePipeline::ImageSequenceLoadHandle(instance);
    }

    void Scene::DispatchAsyncSceneLoads()
    {
        m_sceneAssetManager.DispatchCallbacks();
        RenderUpload::UploadBudget budget;
        budget.maxTaskCount = 2;
        budget.maxTextureCount = 2;
        budget.maxBufferCount = 4;
        budget.maxBytes = 8 * 1024 * 1024;
		for (std::size_t i = 0; i < m_sceneAsyncLoads.size();) {
			std::shared_ptr<SceneAsyncLoadInstance>& instance = m_sceneAsyncLoads[i];
			instance->SubmitTextureUploads(budget);
			instance->Tick();
			if (instance->IsDone()) {
				m_sceneAsyncLoads.erase(m_sceneAsyncLoads.begin() + static_cast<std::ptrdiff_t>(i));
			}
			else {
				++i;
			}
		}
    }

    void Scene::DispatchGLTFAsyncLoads()
    {
        DispatchAsyncSceneLoads();
    }

    void Scene::DispatchImageSequenceAsyncLoads()
    {
        m_sceneAssetManager.DispatchCallbacks();
        RenderUpload::UploadBudget budget;
        budget.maxTaskCount = 2;
        budget.maxTextureCount = 2;
        budget.maxBufferCount = 4;
        budget.maxBytes = 8 * 1024 * 1024;
        for (std::size_t i = 0; i < m_imageSequenceAsyncLoads.size();) {
            std::shared_ptr<ImagePipeline::ImageSequenceLoadInstance>& instance = m_imageSequenceAsyncLoads[i];
            instance->SubmitTextureUploads(budget);
            instance->Tick();
            if (instance->IsDone()) {
                m_imageSequenceAsyncLoads.erase(m_imageSequenceAsyncLoads.begin() + static_cast<std::ptrdiff_t>(i));
            } else {
                ++i;
            }
        }
    }
    
    std::shared_ptr<Entity> Scene::GetEntity(const Entity* obj)
    {
        for (int i = 0; i < m_added_objects.size(); ++i)
        {
            const auto& obj_ref = m_added_objects[i];
            if (obj_ref.get() == obj)
            {
                return obj_ref;
            }
        }
        
        std::shared_ptr<Entity>* find;
        std::map<uint32_t, std::shared_ptr<Entity>>::iterator it = m_objects.find(obj->GetId());
        if (it != m_objects.end())
        {
            find = &it->second;
            return *find;
        }
        
        return std::shared_ptr<Entity>();
    }
}
