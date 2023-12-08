/*
* Viry3D
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
#include <assert.h>
#include <memory>

namespace moonriver
{
	class Debug
	{
	public:
		static void LogString(const std::string& str, bool end_line);
	};

    template<typename ... Args>
    static std::string str_format(const std::string& format, Args ... args)
    {
        auto size_buf = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
        std::unique_ptr<char[]> buf(new(std::nothrow) char[size_buf]);

        if (!buf)
            return std::string("");

        std::snprintf(buf.get(), size_buf, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size_buf - 1);
    }

#define Log(...) moonriver::Debug::LogString(str_format(__VA_ARGS__) + str_format("\n<=[%s:%d]", __FILE__, __LINE__), true)
                
#define LogGLError()						\
    {										\
        int err = glGetError();				\
		if(err != 0)						\
		{									\
            Log("glGetError: %d", err);		\
        }									\
    }
}
