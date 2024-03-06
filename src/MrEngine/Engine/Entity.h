#pragma once
#include "Object.h"
#include <memory>
#include "Component.h"
#include "math/Bounds.h"
#include "Transform.h"

namespace moonriver
{
class Scene;
class Entity :
    public Object, public std::enable_shared_from_this<Entity>
{
public:
    static std::shared_ptr<Entity> Create(const std::string& name);
    static void Destroy(std::shared_ptr<Entity>& obj);
    template <class T, typename ...ARGS> std::shared_ptr<T> AddComponent(ARGS... args);
    template <class T> std::shared_ptr<T> GetComponent() const;
    template <class T> std::vector<std::shared_ptr<T>> GetComponents() const;
    template <class T> std::vector<std::shared_ptr<T>> GetComponentsInChildren() const;
    void RemoveComponent(const std::shared_ptr<Component>& com);
    const std::shared_ptr<Transform>& GetTransform() const { return m_transform; }
    int GetLayer() const { return m_layer; }
    void SetLayer(int layer);
    bool IsActiveSelf() const { return m_is_active_self; }
    void SetActive(bool active);
    bool IsActiveInTree() const { return m_is_active_in_tree; }
    void Update();
    void LateUpdate();
    void AddInWorldRecursively(Scene* pScene);
    void AddEntityTreeInScene(Scene* pScene);
    AABB RecalculateBoundsInternal();
    ~Entity();
    Entity(const std::string& name);
    std::weak_ptr<Scene> m_Scene;
    std::shared_ptr<Transform> m_transform;
private:
    friend class Transform;
    void OnTransformDirty();
    std::vector<std::shared_ptr<Component>> m_components;
    std::vector<std::shared_ptr<Component>> m_added_components;
    std::vector<std::shared_ptr<Component>> m_removed_components;
    int m_layer;
    bool m_is_active_self;
    bool m_is_active_in_tree;
};

    template <class T, typename ...ARGS>
    std::shared_ptr<T> Entity::AddComponent(ARGS... args)
    {
        std::shared_ptr<T> com = std::make_shared<T>(args...);

        auto is_transform = std::dynamic_pointer_cast<Transform>(com);
        if (m_transform && is_transform)
        {
            return std::shared_ptr<T>();
        }
        m_added_components.push_back(com);
        com->m_Entity = shared_from_this();
        com->SetName(GetName());
        return com;
    }

    template <class T>
    std::shared_ptr<T> Entity::GetComponent() const
    {
        for (int i = 0; i < m_added_components.size(); ++i)
        {
            auto& com = m_added_components[i];
            auto t = std::dynamic_pointer_cast<T>(com);
            if (t)
            {
                return t;
            }
        }

        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];
            auto t = std::dynamic_pointer_cast<T>(com);
            if (t)
            {
                return t;
            }
        }

        return std::shared_ptr<T>();
    }

    template <class T>
    std::vector<std::shared_ptr<T>> Entity::GetComponents() const
    {
        std::vector<std::shared_ptr<T>> coms;

        for (int i = 0; i < m_added_components.size(); ++i)
        {
            auto& com = m_added_components[i];
            auto t = std::dynamic_pointer_cast<T>(com);
            if (t)
            {
                coms.push_back(t);
            }
        }

        for (int i = 0; i < m_components.size(); ++i)
        {
            auto& com = m_components[i];
            auto t = std::dynamic_pointer_cast<T>(com);
            if (t)
            {
                coms.push_back(t);
            }
        }

        return coms;
    }

    template <class T>
    std::vector<std::shared_ptr<T>> Entity::GetComponentsInChildren() const
    {
        std::vector<std::shared_ptr<T>> coms = this->GetComponents<T>();

        int child_count = this->GetTransform()->GetChildCount();
        for (int i = 0; i < child_count; ++i)
        {
            auto child = this->GetTransform()->GetChild(i);
            std::vector<std::shared_ptr<T>> child_coms = child->GetEntity()->GetComponentsInChildren<T>();
            for (int j = 0; j < child_coms.size(); ++j)
            {
                coms.push_back(child_coms[j]);
            }
        }

        return coms;
    }
}