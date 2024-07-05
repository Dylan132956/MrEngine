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
#include <memory>
#include <string>
#include "memory/ByteBuffer.h"
#include "Object.h"

namespace moonriver
{
    enum class ImageFormat
    {
        None = 0,
        R8,
        R8G8B8,
        R8G8B8A8,
        YUV420P,
    };

	class Image : public Object
	{
	public:
        static std::shared_ptr<Image> LoadFromFile(const std::string& path);
		static std::shared_ptr<Image> LoadFromMemory(const ByteBuffer& buffer);
		static std::shared_ptr<Image> LoadJPEG(const ByteBuffer& jpeg);
		static std::shared_ptr<Image> LoadPNG(const ByteBuffer& png);
		void EncodeToPNG(const std::string& file);
		void Clear();

        int width = 0;
        int height = 0;
        ImageFormat format = ImageFormat::None;
        ByteBuffer data;
	};
}
