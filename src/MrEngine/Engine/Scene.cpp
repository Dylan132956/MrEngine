
#include "Scene.h"
#include "Entity.h"
#include "Transform.h"

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
