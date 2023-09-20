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
}
