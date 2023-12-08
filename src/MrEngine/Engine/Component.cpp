#include "Component.h"
#include "Entity.h"

namespace moonriver
{
    Component::Component()
    {

    }

    Component::~Component()
    {

    }

    const std::shared_ptr<Transform>& Component::GetTransform() const
    {
        return this->GetEntity()->GetTransform();
    }

    void Component::Enable(bool enable)
    {
        if (m_enable != enable)
        {
            m_enable = enable;

            this->OnEnable(m_enable);
        }
    }
}