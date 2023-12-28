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

#include "private/backend/DriverApi.h"
#include <functional>
#include "memory/Ref.h"
#include "Object.h"
#include <string>

using namespace std;

namespace moonriver
{
	class ByteBuffer;
	class Image;

	enum class CubemapFace
	{
		Unknown = -1,

		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ,

		Count
	};

	enum class TextureFormat
	{
		None,
		R8,
		R8G8B8A8,
        R16F,
        R16G16B16A16F,
        R32F,
        R32G32B32A32F,
		D16,
		D24X8,
		D32,
		D24S8,
		D32S8,
        ETC1_R8G8B8,
        PVRTC_R8G8B8_4V1,
        PVRTC_R8G8B8A8_4V1,
		BC1_RGB,
		BC1_RGBA,
		BC2,
		BC3,
		ETC2_R8G8B8,
		ETC2_R8G8B8A1,
		ETC2_R8G8B8A8,
		ASTC_4x4,
	};

	enum class FilterMode
	{
		None = -1,
		Nearest,
		Linear,
		Trilinear,
	};

	enum class SamplerAddressMode
	{
		None = -1,
		Repeat,
		ClampToEdge,
		Mirror,
		MirrorOnce,
	};

    class Texture : public Object
    {
    public:
        static void Init();
        static void Done();
		static const Ref<Image>& GetSharedWhiteImage();
        static const Ref<Texture>& GetSharedWhiteTexture();
        static const Ref<Texture>& GetSharedBlackTexture();
        static const Ref<Texture>& GetSharedGreenTexture();
        static const Ref<Texture>& GetSharedNormalTexture();
        static const Ref<Texture>& GetSharedCubemap();
		static Ref<Texture> LoadFromKTXFile(
			const string& path,
			FilterMode filter_mode,
			SamplerAddressMode wrap_mode);
        static Ref<Texture> LoadTexture2DFromFile(
            const string& path,
            FilterMode filter_mode,
            SamplerAddressMode wrap_mode,
            bool gen_mipmap);
		static Ref<Texture> LoadTexture2DFromMemory(
			const ByteBuffer& image_buffer,
			FilterMode filter_mode,
			SamplerAddressMode wrap_mode,
			bool gen_mipmap);
        static Ref<Texture> CreateTexture2DFromMemory(
            const ByteBuffer& pixels,
            int width,
            int height,
            TextureFormat format,
            FilterMode filter_mode,
            SamplerAddressMode wrap_mode,
            bool gen_mipmap);
        static Ref<Texture> CreateTexture2D(
            int width,
            int height,
            TextureFormat format,
            FilterMode filter_mode,
            SamplerAddressMode wrap_mode,
            bool mipmap);
        static Ref<Texture> CreateCubemap(
            int size,
            TextureFormat format,
            FilterMode filter_mode,
            SamplerAddressMode wrap_mode,
            bool mipmap);
		static Ref<Texture> CreateRenderTexture(
			int width,
			int height,
			TextureFormat format,
			FilterMode filter_mode,
			SamplerAddressMode wrap_mode);
		static TextureFormat SelectFormat(const vector<TextureFormat>& formats, bool render_texture);
		static TextureFormat SelectDepthFormat();
        virtual ~Texture();
		void UpdateCubemap(const ByteBuffer& pixels, int level, const vector<int>& face_offsets);
		void UpdateTexture(const ByteBuffer& pixels, int layer, int level, int x, int y, int w, int h);
		void CopyTexture(
			int dst_layer, int dst_level,
			int dst_x, int dst_y,
			int dst_w, int dst_h,
			const Ref<Texture>& src,
			int src_layer, int src_level,
			int src_x, int src_y,
			int src_w, int src_h,
			FilterMode blit_filter);
		void CopyToMemory(
			ByteBuffer& pixels,
			int layer, int level,
			int x, int y,
			int w, int h,
			std::function<void(const ByteBuffer&)> on_complete);
        void GenMipmaps();
		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		TextureFormat GetFormat() const { return m_format; }
        int GetMipmapLevelCount() const { return m_mipmap_level_count; }
        int GetArraySize() const { return m_array_size; }
        bool IsCubemap() const { return m_cubemap; }
        FilterMode GetFilterMode() const { return m_filter_mode; }
        SamplerAddressMode GetSamplerAddressMode() const { return m_wrap_mode; }
        const filament::backend::TextureHandle& GetTexture() const { return m_texture; }
        const filament::backend::SamplerParams& GetSampler() const { return m_sampler; }

    private:
        Texture();
        void UpdateSampler(bool depth);
        
	private:
		static Ref<Image> m_shared_white_image;
        static Ref<Texture> m_shared_white_texture;
        static Ref<Texture> m_shared_black_texture;
        static Ref<Texture> m_shared_green_texture;
        static Ref<Texture> m_shared_normal_texture;
        static Ref<Texture> m_shared_cubemap;
		int m_width;
		int m_height;
        int m_mipmap_level_count;
        int m_array_size;
        bool m_cubemap;
        TextureFormat m_format;
        FilterMode m_filter_mode;
        SamplerAddressMode m_wrap_mode;
        filament::backend::TextureHandle m_texture;
        filament::backend::SamplerParams m_sampler;
		string m_file_path;
    };
}
