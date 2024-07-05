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

namespace moonriver
{
    struct Vector2i
    {
        Vector2i(int x = 0, int y = 0): x(x), y(y) { }

        Vector2i operator +(const Vector2i& right) const
        {
            return Vector2i(x + right.x, y + right.y);
        }

        Vector2i& operator +=(const Vector2i& right)
        {
            *this = *this + right;
            return *this;
        }

        Vector2i operator -(const Vector2i& right) const
        {
            return Vector2i(x - right.x, y - right.y);
        }

        Vector2i& operator -=(const Vector2i& right)
        {
            *this = *this - right;
            return *this;
        }

        bool operator ==(const Vector2i& right) const
        {
            return x == right.x && y == right.y;
        }

        bool operator !=(const Vector2i& right) const
        {
            return x != right.x || y != right.y;
        }

        int x;
        int y;
    };
}
