#pragma once
#include "Object.h"
#include <memory>

namespace moonriver
{
class Entity;
class Transform;
class Component :
    public Object
{
public:
    Component();
    virtual ~Component();
    std::shared_ptr<Entity> GetEntity() const { return m_Entity.lock(); }
    const std::shared_ptr<Transform>& GetTransform() const;
    void Enable(bool enable);
    bool IsEnable() const { return m_enable; }
protected:
    virtual void Start() { }
    virtual void Update() { }
    virtual void LateUpdate() { }
    virtual void OnTransformDirty() { }
    virtual void OnEnable(bool enable) { }
    virtual void OnGameObjectLayerChanged() { }
    virtual void OnGameObjectActiveChanged() { }

private:
    friend class Entity;
    std::weak_ptr<Entity> m_Entity;
    bool m_enable = true;
    bool m_started = false;
};

}

