#pragma once

#include "Object.h"
#include <memory>
#include <map>

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
        std::shared_ptr<Entity> GetEntity(const Entity* obj);
        std::shared_ptr<Entity> CreateEntity(const char* name);
        std::shared_ptr<Entity> LoadGLTF(const char* name);
        void AddEntity(const std::shared_ptr<Entity>& obj);
        void RemoveEntity(const std::shared_ptr<Entity>& obj);
	private:
		//static Scene* m_instance;
	    std::map<uint32_t, std::shared_ptr<Entity>> m_objects;
		std::vector<std::shared_ptr<Entity>> m_added_objects;
		std::vector<std::shared_ptr<Entity>> m_removed_objects;
    };
}
