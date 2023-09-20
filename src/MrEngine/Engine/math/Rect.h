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

namespace moonriver
{
	struct Rect
	{
        static Rect Max(const Rect& a, const Rect& b);
        static Rect Min(const Rect& a, const Rect& b);

		explicit Rect(float x = 0, float y = 0, float w = 0, float h = 0):
            x(x),
            y(y),
			w(w),
			h(h)
		{
		}

		void Set(float x, float y, float w, float h)
		{
			this->x = x;
			this->y = y;
			this->w = w;
			this->h = h;
		}

		bool operator ==(const Rect &r) const;
		bool operator !=(const Rect &r) const;

		float x;
		float y;
		float w;
		float h;
	};
}
