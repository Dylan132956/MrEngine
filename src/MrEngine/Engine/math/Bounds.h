/*
* moonriver
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "Matrix4x4.h"
#include "Vector3.h"

namespace moonriver
{
	class Bounds
	{
	public:
        Bounds();
		Bounds(const Vector3& min, const Vector3& max);
		const Vector3& Min() const { return m_min; }
		const Vector3& Max() const { return m_max; }
        Vector3 GetCenter() const { return (m_min + m_max) * 0.5f; }
        Vector3 GetSize() const { return m_max - m_min; }
		bool Contains(const Vector3& point) const;

	private:
		Vector3 m_min;
		Vector3 m_max;
	};

	const Vector3  infinityVec = Vector3(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());

	inline Vector3 min(const Vector3& lhs, const Vector3& rhs) { return Vector3(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y), std::min(lhs.z, rhs.z)); }

	inline Vector3 max(const Vector3& lhs, const Vector3& rhs) { return Vector3(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y), std::max(lhs.z, rhs.z)); }

	class MinMaxAABB
	{
	public:
		MinMaxAABB() { Init(); }
		MinMaxAABB(const Vector3& min, const Vector3& max) : m_Min(min), m_Max(max) {}
		inline void Init()
		{
			m_Min = infinityVec;
			m_Max = -infinityVec;
		}

		Vector3    m_Min;
		Vector3    m_Max;
		void Encapsulate(const Vector3& inPoint)
		{
			m_Min = min(m_Min, inPoint);
			m_Max = max(m_Max, inPoint);
		}
		bool IsValid() const;
	};

	inline bool MinMaxAABB::IsValid() const
	{
		return !(m_Min == infinityVec || m_Max == -infinityVec);

		Matrix4x4 a;
	}

	class AABB
	{
	public:

		Vector3    m_Center;
		Vector3    m_Extent;
		AABB() {}
		explicit AABB(const MinMaxAABB& aabb) { FromMinMaxAABB(aabb); }
		AABB(const Vector3& center, const Vector3& extent) { m_Center = center; m_Extent = extent; }
		void SetCenterAndExtent(const Vector3& center, const Vector3& extent) { m_Center = center; m_Extent = extent; }
		Vector3 CalculateMin() const { return m_Center - m_Extent; }
		Vector3 CalculateMax() const { return m_Center + m_Extent; }
		inline void FromMinMaxAABB(const MinMaxAABB& inAABB)
		{
			m_Center = (inAABB.m_Min + inAABB.m_Max) * 0.5F;
			m_Extent = (inAABB.m_Max - inAABB.m_Min) * 0.5F;
		}

		Vector3& GetCenter() { return m_Center; }
		Vector3& GetExtent() { return m_Extent; }
		const Vector3& GetCenter() const { return m_Center; }
		const Vector3& GetExtent() const { return m_Extent; }
	};

	inline Vector3 RotateExtents(const Vector3& extents, const Matrix4x4& rotation)
	{
		float matrix[4][4];
		memcpy(matrix, &rotation, sizeof(matrix));
		Vector3 newExtents;
		for (int i = 0; i < 3; i++)
		{
			//newExtents[i] = fabs(rotation.m[i][0] * extents.x) + fabs(rotation.c[i][1] * extents.y) + fabs(rotation.c[i][2] * extents.z);
			newExtents[i] = fabs(matrix[0][i] * extents.x) + fabs(matrix[1][i] * extents.y) + fabs(matrix[2][i] * extents.z);
		}
		return newExtents;
	}

	inline void TransformAABB(const AABB& aabb, const Matrix4x4& transform, AABB& result)
	{
		//ASSERT_VALID_AABB(aabb);
		Vector3 extents = RotateExtents(aabb.m_Extent, transform);
		Vector3 center = transform.MultiplyPoint3x4(aabb.m_Center);
		result.SetCenterAndExtent(center, extents);
		//ASSERT_VALID_AABB(result);
	}
}


