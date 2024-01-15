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

#include <string>

namespace moonriver
{
	struct Vector2;
    struct Vector4;

	struct Vector3
	{
		static const Vector3& Zero();
		static const Vector3& One();
		static Vector3 Normalize(const Vector3& value);
		static float Magnitude(const Vector3& v);
		static float SqrMagnitude(const Vector3& v);
        static float Distance(const Vector3& a, const Vector3& b);
		static Vector3 Max(const Vector3& a, const Vector3& b);
		static Vector3 Min(const Vector3& a, const Vector3& b);
		static Vector3 Lerp(const Vector3& from, const Vector3& to, float t, bool clamp_01 = true);
		static float Angle(const Vector3& from, const Vector3& to);
        static float Dot(const Vector3& a, const Vector3& b);

		Vector3(float x = 0, float y = 0, float z = 0);
        Vector3(const Vector4& v4);
		Vector3(const Vector2& v2);
		Vector3 operator -() const;
		Vector3 operator +(const Vector3& v) const;
		Vector3& operator +=(const Vector3& v);
		Vector3 operator -(const Vector3& v) const;
		Vector3 operator *(const Vector3& v) const;
		Vector3 operator *(float v) const;
		Vector3 operator *=(float v);
        Vector3 operator /(float v) const;
        Vector3 operator /=(float v);
		float& operator[](int index)
		{
			return *(&x + index);
		}
		bool operator !=(const Vector3& v) const;
		bool operator ==(const Vector3& v) const;
		float Dot(const Vector3& v) const;
		std::string ToString() const;
		void Normalize();
        Vector3 Normalized() const;
		float Magnitude() const;
		float SqrMagnitude() const;

        template <typename Y>
        static Vector3 MakeVector(const Y& vals)
        {
            return Vector3 //
            {
                static_cast<float>(vals[0]),
                static_cast<float>(vals[1]),
                static_cast<float>(vals[2]) //
            };
        }

		float x;
		float y;
		float z;
	};
}
