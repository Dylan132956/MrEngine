/*
* moonriver
* Copyright 2014-2019 by Dylan - 13227110@qq.com
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

#include "D3D12Driver.h"
#include "D3D12Context.h"
#include "d3d12shader.h"
#include "D3D12Buffer.h"
#include <map>

namespace filament
{
	namespace backend
	{
		struct D3D12SwapChain : public HwSwapChain 
		{
			D3D12SwapChain(D3D12Context* context, void* native_window);
			~D3D12SwapChain();
			ComPtr<IDXGISwapChain3> m_swapChain;
			static const int SwapChainBufferCount = 2;
			SwapChainBuffer m_backbuffers[SwapChainBufferCount];
			int mCurrBackBuffer = 0;
		};

		struct D3D12RenderTarget : public HwRenderTarget
		{
			D3D12RenderTarget(
				D3D12Context* context,
				TargetBufferFlags flags,
				uint32_t width,
				uint32_t height,
				uint8_t samples,
				TargetBufferInfo color,
				TargetBufferInfo depth,
				TargetBufferInfo stencil);
			explicit D3D12RenderTarget(D3D12Context* context);
			~D3D12RenderTarget();
			void CreateDepth(D3D12Context* context, DXGI_FORMAT format, uint32_t samples, uint32_t width, uint32_t height);

			bool default_render_target = false;

			ComPtr<ID3D12Resource> colorTexture;
			ComPtr<ID3D12Resource> depthStencilTexture;
			std::unique_ptr<MD3D12ShaderResourceView> SRV;
			std::unique_ptr<MD3D12DepthStencilView> DSV;
			UINT width, height;
			UINT samples;
		};

		struct D3D12Program : public HwProgram
		{
			D3D12Program(D3D12Context* context, Program&& program);
			~D3D12Program();

			Program info;
			ID3DBlob* vertex_binary = nullptr;
			ID3DBlob* pixel_binary = nullptr;
			std::vector<D3D12_SHADER_INPUT_BIND_DESC> vert_cbv;
			std::vector<D3D12_SHADER_INPUT_BIND_DESC> vert_srv;
			std::vector<D3D12_SHADER_INPUT_BIND_DESC> vert_sam;

			std::vector<D3D12_SHADER_INPUT_BIND_DESC> frag_cbv;
			std::vector<D3D12_SHADER_INPUT_BIND_DESC> frag_srv;
			std::vector<D3D12_SHADER_INPUT_BIND_DESC> frag_sam;

			std::map<UINT, UINT> vert_cbv_map;
			std::map<UINT, UINT> vert_srv_map;
			std::map<UINT, UINT> frag_cbv_map;
			std::map<UINT, UINT> frag_srv_map;

			std::vector<D3D12_INPUT_ELEMENT_DESC> meshInputLayout;

			ComPtr<ID3D12PipelineState> GetPSO(D3D12Context* context, const backend::PipelineState& ps);

			ComPtr<ID3D12RootSignature> m_RootSignature;
			ComPtr<ID3D12PipelineState> m_PipelineState;
		};

		struct D3D12UniformBuffer : public HwUniformBuffer
		{
			D3D12UniformBuffer(D3D12Context* context, size_t size, BufferUsage usage);
			~D3D12UniformBuffer();
			void Load(D3D12Context* context, const BufferDescriptor& data);
			MD3D12ConstantBufferRef cb;
			size_t size;
			BufferUsage usage;
			template<typename T> T* as() const
			{
				return reinterpret_cast<T*>(data.cpuAddress);
			}
		};

		struct D3D12SamplerGroup : public HwSamplerGroup
		{
			D3D12SamplerGroup(D3D12Context* context, size_t size);
			~D3D12SamplerGroup();
			void Update(D3D12Context* context, SamplerGroup&& sg);
		};

		struct D3D12Texture : public HwTexture
		{
			D3D12Texture(
				D3D12Context* context,
				backend::SamplerType target,
				uint8_t levels,
				TextureFormat format,
				uint8_t samples,
				uint32_t width,
				uint32_t height,
				uint32_t depth,
				TextureUsage usage);
			~D3D12Texture();
			void createTextureSRV(
				D3D12Context* context, 
				D3D12_SRV_DIMENSION dimension, 
				UINT mostDetailedMip=0,
				UINT mipLevels=0);
			void createTextureUAV(D3D12Context* context, UINT mipSlice);
			void CreateDepth(
				D3D12Context* context, 
				DXGI_FORMAT format, 
				uint32_t samples, 
				uint32_t width, 
				uint32_t height);
			void UpdateTexture(
				D3D12Context* context,
				int layer, int level,
				int x, int y,
				int w, int h,
				const PixelBufferDescriptor& data);
			void CopyTexture(
				D3D12Context* context,
				int dst_layer, int dst_level,
				const backend::Offset3D& dst_offset,
				const backend::Offset3D& dst_extent,
				D3D12Texture* src,
				int src_layer, int src_level,
				const backend::Offset3D& src_offset,
				const backend::Offset3D& src_extent);
			void CopyTextureToMemory(
				D3D12Context* context,
				int layer, int level,
				const Offset3D& offset,
				const Offset3D& extent,
				PixelBufferDescriptor& data);
			void GenerateMipmaps(D3D12Context* context);

			ComPtr<ID3D12Resource> texture;

			std::unique_ptr<MD3D12ShaderResourceView> SRV;
			std::unique_ptr<MD3D12DepthStencilView> DSV;
			std::unique_ptr<MD3D12UnorderedAccessView> UAV;
		};

		struct D3D12VertexBuffer : public HwVertexBuffer
		{
			D3D12VertexBuffer(
				D3D12Context* context,
				uint8_t buffer_count,
				uint8_t attribute_count,
				uint32_t vertex_count,
				AttributeArray attributes,
				BufferUsage usage);
			~D3D12VertexBuffer();
			void Update(
				D3D12Context* context,
				size_t index,
				const BufferDescriptor& data,
				uint32_t offset);

			std::vector<MD3D12VertexBufferRef> VertexBufferRefArray;
			uint32_t stride;
			UINT size;
			BufferUsage usage;
		};

		struct D3D12IndexBuffer : public HwIndexBuffer
		{
			D3D12IndexBuffer(
				D3D12Context* context,
				ElementType element_type,
				uint32_t index_count,
				BufferUsage usage);
			~D3D12IndexBuffer();
			void Update(
				D3D12Context* context,
				const BufferDescriptor& data,
				uint32_t offset);

			MD3D12IndexBufferRef IndexBufferRef;

			ComPtr<ID3D12Resource> buffer;
			size_t indexDataSize;
			ElementType indexElement_type;
			DXGI_FORMAT indexFormat;
			BufferUsage usage;
		};

		struct D3D12RenderPrimitive : public HwRenderPrimitive
		{
			D3D12RenderPrimitive(D3D12Context* context);
			~D3D12RenderPrimitive();
			void SetBuffer(
				D3D12Context* context,
				Handle<HwVertexBuffer> vbh,
				Handle<HwIndexBuffer> ibh,
				uint32_t enabled_attributes,
				uint32_t vertex_count);
			void SetRange(
				D3D12Context* context,
				PrimitiveType pt,
				uint32_t offset,
				uint32_t min_index,
				uint32_t max_index,
				uint32_t count);

			VertexBufferHandle vertex_buffer;
			IndexBufferHandle index_buffer;
			uint32_t enabled_attributes = 0;
		};
	}
}
