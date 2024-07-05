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

#include "Vector2.h"
#include "Vector3.h"
#include "Mathf.h"
#include <sstream>

namespace moonriver
{
    const Vector2& Vector2::One()
    {
        static const Vector2 s_one(1, 1);
        return s_one;
    }

    const Vector2& Vector2::Zero()
    {
        static const Vector2 s_zero(0, 0);
        return s_zero;
    }

    Vector2 Vector2::Lerp(const Vector2& from, const Vector2& to, float t, bool clamp_01)
    {
        return Vector2(
            Mathf::Lerp(from.x, to.x, t, clamp_01),
            Mathf::Lerp(from.y, to.y, t, clamp_01));
    }

	Vector2::Vector2(const Vector3& v3):
		x(v3.x),
		y(v3.y)
	{
	}

	Vector2 Vector2::operator *(float value) const
	{
		return Vector2(x * value, y * value);
	}

	Vector2 Vector2::operator +(const Vector2& value) const
	{
		return Vector2(x + value.x, y + value.y);
	}

	Vector2 Vector2::operator -(const Vector2& value) const
	{
		return Vector2(x - value.x, y - value.y);
	}

    Vector2& Vector2::operator *=(float value)
    {
        x *= value;
        y *= value;
        return *this;
    }

    Vector2& Vector2::operator +=(const Vector2& value)
    {
        x += value.x;
        y += value.y;
        return *this;
    }

    Vector2& Vector2::operator -=(const Vector2& value)
    {
        x -= value.x;
        y -= value.y;
        return *this;
    }

	float Vector2::Magnitude() const
	{
		return sqrt(SqrMagnitude());
	}

	float Vector2::SqrMagnitude() const
	{
		return x * x + y * y;
	}

	bool Vector2::operator ==(const Vector2& value) const
	{
		return Mathf::FloatEqual(x, value.x) && Mathf::FloatEqual(y, value.y);
	}

	bool Vector2::operator !=(const Vector2& value) const
	{
		return !(*this == value);
	}

	std::string Vector2::ToString() const
	{
		std::stringstream ss;
		ss << '(' << x << ',' << y << ')';
		auto str = ss.str();
		return str.c_str();
	}
}
