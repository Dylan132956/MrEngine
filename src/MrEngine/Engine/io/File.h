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
#include "memory/ByteBuffer.h"

namespace moonriver
{
	class File
	{
	public:
		static bool Exist(const std::string& path);
		static ByteBuffer ReadAllBytes(const std::string& path);
		static bool WriteAllBytes(const std::string& path, const ByteBuffer& buffer);
		static std::string ReadAllText(const std::string& path);
		static bool WriteAllText(const std::string& path, const std::string& text);
        static void Delete(const std::string& path);
		static void Unzip(const std::string& path, const std::string& source, const std::string& dest, bool directory);
	};
}
