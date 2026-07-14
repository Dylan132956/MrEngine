#pragma once

#include "Object.h"
#include <memory>
#include <map>
#include <vector>
#include "Entity.h"
#include "asset/AssetManager.h"
#include "renderupload/FrameUploadScheduler.h"
#include "sceneasync.h"
#include "asset/image/ImageSequence.h"

namespace moonriver
{
	class Entity;

    class Scene : public Object, public std::enable_shared_from_this<Scene>
    {
    public:
		//static Scene* Instance();
        Scene();
        virtual ~Scene();
        void Update();
        void Render();
        std::shared_ptr<Entity> GetEntity(const Entity* obj);
        std::shared_ptr<Entity> CreateEntity(const char* name);
        std::shared_ptr<Entity> LoadGLTF(const char* name);
        SceneAsyncLoadHandle LoadSceneAsync(const char* name);
        GLTFAsyncLoadHandle LoadGLTFAsync(const char* name);
        ImagePipeline::ImageSequenceLoadHandle LoadImageSequenceAsync(const char* name,
                                                                      const std::vector<std::string>& frameRelativePaths,
                                                                      float fps,
                                                                      bool loop);
        void DispatchAsyncSceneLoads();
        void DispatchGLTFAsyncLoads();
        void DispatchImageSequenceAsyncLoads();
        void AddEntity(const std::shared_ptr<Entity>& obj);
        void RemoveEntity(const std::shared_ptr<Entity>& obj);
	private:
		//static Scene* m_instance;
	    std::map<uint32_t, std::shared_ptr<Entity>> m_objects;
		std::vector<std::shared_ptr<Entity>> m_added_objects;
		std::vector<std::shared_ptr<Entity>> m_removed_objects;
        AsyncLoad::AssetManager m_sceneAssetManager;
        RenderUpload::FrameUploadScheduler m_sceneUploadScheduler;
        std::vector<std::shared_ptr<class SceneAsyncLoadInstance> > m_sceneAsyncLoads;
        std::vector<std::shared_ptr<ImagePipeline::ImageSequenceLoadInstance> > m_imageSequenceAsyncLoads;
    };
}
