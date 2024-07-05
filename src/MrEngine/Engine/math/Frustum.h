/*
* moonriver
* Copyright 2023-2024 by Dylan - 13227110@qq.com
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
	enum class ContainsResult
	{
		In,
		Out,
		Cross
	};

	class Frustum
	{
	public:
		Frustum() { }

		//
		// ժҪ:
		//     ///
		//     ����һ��FrustumBounds, ͶӰ����Ϊperspective.
		//     ///
		//
		// ����:
		//   mat:
		//     wvp matrix, check pos in model space.
		//     vp matrix, check pos in world space.
		//     p matrix, check pos in view space.
		Frustum(const Matrix4x4& mat);

		//
		// ժҪ:
		//     ///
		//     ����һ��FrustumBounds, ͶӰ����Ϊorthographic, in view space.
		//     ///
		Frustum(float left, float right, float bottom, float top, float near, float far);

		ContainsResult ContainsPoint(const Vector3& point) const;
		ContainsResult ContainsSphere(const Vector3& center, float radius) const;
		ContainsResult ContainsBounds(const Vector3& min, const Vector3& max) const;
		ContainsResult ContainsPoints(const std::vector<Vector3>& points, const Matrix4x4* matrix) const;
		float DistanceToPlane(const Vector3& point, int plane_index) const;

	private:
		void NormalizePlanes();

		Vector4 m_planes[6];
	};
}
