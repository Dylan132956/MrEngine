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

#include <string>

namespace moonriver
{
	struct Vector3;

	struct Vector2
	{
		static const Vector2& One();
		static const Vector2& Zero();
        static Vector2 Lerp(const Vector2& from, const Vector2& to, float t, bool clamp_01 = true);

		Vector2(float x = 0, float y = 0): x(x), y(y) { }
		Vector2(const Vector3& v3);
		Vector2 operator +(const Vector2& value) const;
        Vector2& operator +=(const Vector2& value);
		Vector2 operator -(const Vector2& value) const;
        Vector2& operator -=(const Vector2& value);
        float operator *(const Vector2& v) const { return x * v.y - y * v.x; }
        Vector2 operator *(float value) const;
        Vector2& operator *=(float value);
		bool operator ==(const Vector2& value) const;
		bool operator !=(const Vector2& value) const;
        float Dot(const Vector2& v) const { return x * v.x + y * v.y; }
		std::string ToString() const;
		float Magnitude() const;
		float SqrMagnitude() const;

        template <typename Y>
        static Vector2 MakeVector(const Y& vals)
        {
            return Vector2 //
            {
                static_cast<float>(vals[0]),
                static_cast<float>(vals[1]) //
            };
        }

		float x;
		float y;
	};
}
