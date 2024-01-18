
#include "Transform.h"
#include "Entity.h"
#include "string/stringutils.h"

namespace moonriver
{
	Transform::Transform():
		m_local_position(0, 0, 0),
		m_local_rotation(Quaternion::Identity()),
		m_local_scale(1, 1, 1),
		m_position(m_local_position),
		m_rotation(m_local_rotation),
		m_scale(m_local_scale),
		m_local_to_world(Matrix4x4::Identity()),
		m_world_to_local(Matrix4x4::Identity()),
		m_dirty(false)
	{

	}
    
    Transform::~Transform()
    {
        
    }

	void Transform::SetParent(const std::shared_ptr<Transform>& parent)
	{
        if (m_parent.lock() != parent)
        {
            Vector3 position = this->GetPosition();
            Quaternion rotation = this->GetRotation();
            Vector3 scale = this->GetScale();

            auto old_parent = m_parent.lock();
            if (old_parent)
            {
                for (int i = 0; i < old_parent->GetChildCount(); ++i)
                {
                    if (old_parent->GetChild(i).get() == this)
                    {
                        old_parent->m_children.erase(old_parent->m_children.begin() + i);
                        break;
                    }
                }
                m_parent.reset();
            }

            if (parent)
            {
                parent->m_children.push_back(this->GetEntity()->GetTransform());
                m_parent = parent;
            }

            this->SetPosition(position);
            this->SetRotation(rotation);
            this->SetScale(scale);

            this->GetEntity()->SetActive(this->GetEntity()->IsActiveSelf());
        }
	}

	std::shared_ptr<Transform> Transform::Find(const std::string& path)
	{
		if (path.empty())
		{
            std::shared_ptr<Transform> transform = shared_from_this();
            return transform;
		}

		std::shared_ptr<Transform> find;
		const Transform* p = this;

		auto layers = SplitString(path, '/');
		for (int i = 0; i < layers.size(); ++i)
		{
			bool find_child = false;

            if (layers[i] == "..")
            {
                find_child = true;
                find = p->GetParent();
                p = find.get();
            }
			else
            {
                for (int j = 0; j < p->GetChildCount(); ++j)
                {
                    if (layers[i] == p->GetChild(j)->GetEntity()->GetName())
                    {
                        find_child = true;
                        find = p->GetChild(j);
                        p = find.get();
                        break;
                    }
                }
            }

			if (!find_child)
			{
				return std::shared_ptr<Transform>();
			}
		}

		return find;
	}

	std::shared_ptr<Transform> Transform::GetRoot() const
	{
		auto parent = this->GetParent();
		if (parent)
		{
			return parent->GetRoot();
		}
		else
		{
			return this->GetEntity()->GetTransform();
		}
	}

	void Transform::SetLocalPosition(const Vector3& pos)
	{
        if (m_local_position != pos)
        {
            m_local_position = pos;

            this->MarkDirty();
        }
	}

	void Transform::SetLocalRotation(const Quaternion& rot)
	{
        if (m_local_rotation != rot)
        {
            m_local_rotation = rot;

            this->MarkDirty();
        }
	}

	void Transform::SetLocalScale(const Vector3& scale)
	{
        if (m_local_scale != scale)
        {
            m_local_scale = scale;

            this->MarkDirty();
        }
	}

	const Vector3& Transform::GetPosition()
	{
        this->UpdateMatrix();
        
        return m_position;
	}
    
    void Transform::SetPosition(const Vector3& pos)
    {
		Vector3 local_position;

		auto parent = m_parent.lock();
		if (parent)
		{
			local_position = parent->GetWorldToLocalMatrix().MultiplyPoint3x4(pos);
		}
		else
		{
			local_position = pos;
		}

		this->SetLocalPosition(local_position);
    }

	const Quaternion& Transform::GetRotation()
	{
        this->UpdateMatrix();
        
        return m_rotation;
	}
    
    void Transform::SetRotation(const Quaternion& rot)
    {
        Quaternion local_rotation;
        
        auto parent = m_parent.lock();
        if (parent)
        {
            local_rotation = Quaternion::Inverse(parent->GetRotation()) * rot;
        }
        else
        {
            local_rotation = rot;
        }
        
        this->SetLocalRotation(local_rotation);
    }

	const Vector3& Transform::GetScale()
	{
        this->UpdateMatrix();
        
        return m_scale;
	}

    void Transform::SetScale(const Vector3& scale)
    {
        Vector3 local_scale;
        
        auto parent = m_parent.lock();
        if (parent)
        {
            const auto& parent_scale = parent->GetScale();
            local_scale = Vector3(scale.x / parent_scale.x, scale.y / parent_scale.y, scale.z / parent_scale.z);
        }
        else
        {
            local_scale = scale;
        }
        
        this->SetLocalScale(local_scale);
    }
    
	const Matrix4x4& Transform::GetLocalToWorldMatrix()
	{
		this->UpdateMatrix();
        
        return m_local_to_world;
	}

	const Matrix4x4& Transform::GetWorldToLocalMatrix()
	{
        this->UpdateMatrix();
        
        return m_world_to_local;
	}

	Vector3 Transform::GetRight()
	{
        this->UpdateMatrix();
        
        return m_local_to_world.MultiplyDirection(Vector3(1, 0, 0));
	}

	Vector3 Transform::GetUp()
	{
        this->UpdateMatrix();
        
        return m_local_to_world.MultiplyDirection(Vector3(0, 1, 0));
	}

	Vector3 Transform::GetForward()
	{
        this->UpdateMatrix();
        
        return m_local_to_world.MultiplyDirection(Vector3(0, 0, 1));
	}

	void Transform::MarkDirty()
	{
		m_dirty = true;
		this->GetEntity()->OnTransformDirty();

		for (auto& i : m_children)
		{
			i->MarkDirty();
		}
	}

	void Transform::UpdateMatrix()
	{
		if (m_dirty)
		{
			m_dirty = false;

			auto parent = m_parent.lock();
			if (parent)
			{
				m_local_to_world = parent->GetLocalToWorldMatrix() * Matrix4x4::TRS(m_local_position, m_local_rotation, m_local_scale);
				m_position = parent->GetLocalToWorldMatrix().MultiplyPoint3x4(m_local_position);
				m_rotation = parent->GetRotation() * m_local_rotation;
				const auto& parent_scale = parent->GetScale();
				m_scale = Vector3(parent_scale.x * m_local_scale.x, parent_scale.y * m_local_scale.y, parent_scale.z * m_local_scale.z);
			}
			else
			{
				m_local_to_world = Matrix4x4::TRS(m_local_position, m_local_rotation, m_local_scale);
				m_position = m_local_position;
				m_rotation = m_local_rotation;
				m_scale = m_local_scale;
			}

			m_world_to_local = m_local_to_world.Inverse();
		}
	}
}
