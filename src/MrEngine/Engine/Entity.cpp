#include "Entity.h"
#include "Transform.h"
#include "graphics/MeshRenderer.h"
#include "graphics/Mesh.h"
#include "Scene.h"

namespace moonriver
{
    std::shared_ptr<Entity> Entity::Create(const std::string& name)
    {
        std::shared_ptr<Entity> obj = std::shared_ptr<Entity>(new Entity(name));
        obj->m_transform = obj->AddComponent<Transform>();
        return obj;
    }

    void Entity::Destroy(std::shared_ptr<Entity>& obj)
    {
        obj.reset();
    }

    Entity::Entity(const std::string& name):
        m_layer(0),
        m_is_active_self(true),
        m_is_active_in_tree(true)
    {
        SetName(name);
    }

    Entity::~Entity()
    {
        m_added_components.clear();
        m_removed_components.clear();
        m_components.clear();
        m_transform.reset();
    }

    void Entity::OnTransformDirty()
    {
        for (int i = 0; i < m_added_components.size(); ++i)
        {
            auto& com = m_added_components[i];
            if (com->IsEnable())
            {
                com->OnTransformDirty();
            }
        }

        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];
            if (com->IsEnable())
            {
                com->OnTransformDirty();
            }
        }
    }

    void Entity::SetLayer(int layer)
    {
        if (m_layer != layer)
        {
            m_layer = layer;

            for (int i = 0; i < m_added_components.size(); ++i)
            {
                auto& com = m_added_components[i];
                if (com->IsEnable())
                {
                    com->OnGameObjectLayerChanged();
                }
            }

            for (int i = 0; i < m_components.size(); ++i)
            {
                auto& com = m_components[i];
                if (com->IsEnable())
                {
                    com->OnGameObjectLayerChanged();
                }
            }
        }
    }

    void Entity::SetActive(bool active)
    {
        m_is_active_self = active;

        auto parent = this->GetTransform()->GetParent();
        if (parent)
        {
            m_is_active_in_tree = parent->GetEntity()->IsActiveInTree() && m_is_active_self;
        }
        else
        {
            m_is_active_in_tree = m_is_active_self;
        }

        for (int i = 0; i < m_added_components.size(); ++i)
        {
            auto& com = m_added_components[i];
            if (com->IsEnable())
            {
                com->OnGameObjectActiveChanged();
            }
        }

        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];
            if (com->IsEnable())
            {
                com->OnGameObjectActiveChanged();
            }
        }

        int child_count = this->GetTransform()->GetChildCount();
        for (int i = 0; i < child_count; ++i)
        {
            auto& child = this->GetTransform()->GetChild(i);
            child->GetEntity()->SetActive(child->GetEntity()->IsActiveSelf());
        }
    }

    void Entity::Update()
    {
        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];

            if (com->IsEnable())
            {
                if (!com->m_started)
                {
                    com->m_started = true;
                    com->Start();
                }

                com->Update();
            }
        }

        do
        {
            std::vector<std::shared_ptr<Component>> added = m_added_components;
            m_added_components.clear();

            for (int i = 0; i < added.size(); ++i)
            {
                auto& com = added[i];

                if (com->IsEnable())
                {
                    if (!com->m_started)
                    {
                        com->m_started = true;
                        com->Start();
                    }

                    com->Update();
                }

                m_components.push_back(com);
            }
            added.clear();
        } while (m_added_components.size() > 0);

        for (int i = 0; i < m_removed_components.size(); ++i)
        {
            auto& com = m_removed_components[i];
            m_components.erase(m_components.begin() + i);
        }
        m_removed_components.clear();
    }

    void Entity::LateUpdate()
    {
        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];
            if (com->IsEnable())
            {
                com->LateUpdate();
            }
        }
    }
    //call when world resource loaded
    void Entity::AddInWorldRecursively(Scene* pScene)
    {
        std::shared_ptr<Entity> entity = shared_from_this();
        pScene->AddEntity(entity);
        int childCount = this->GetTransform()->GetChildCount();
        for (int i = 0; i < childCount; ++i)
        {
            std::shared_ptr<Entity> child = GetTransform()->GetChild(i)->GetEntity();
            child->AddInWorldRecursively(pScene);
        }
    }

    void Entity::AddEntityTreeInScene(Scene* pScene)
    {
        AddInWorldRecursively(pScene);
    }

    AABB Entity::RecalculateBoundsInternal()
    {
        MinMaxAABB minmax;
        auto renderers = GetComponentsInChildren<Renderer>();
        for (size_t i = 0; i < renderers.size(); ++i)
        {
            AABB aabb;
            TransformAABB(renderers[i]->GetLocalAABB(), renderers[i]->GetTransform()->GetLocalToWorldMatrix(), aabb);
            minmax.Encapsulate(aabb.CalculateMin());
            minmax.Encapsulate(aabb.CalculateMax());
        }
        return AABB(minmax);
    }

}