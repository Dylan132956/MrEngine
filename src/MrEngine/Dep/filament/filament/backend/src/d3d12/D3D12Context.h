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

#include "private/backend/Driver.h"
#include <unordered_map>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include <wrl.h>
#include "D3D12MemoryAllocator.h"
#include "D3D12HeapSlotAllocator.h"
#include "D3D12Buffer.h"

using Microsoft::WRL::ComPtr;

#define SAFE_RELEASE(p) \
	if (p) \
	{ \
		p->Release(); \
		p = nullptr; \
	}

struct Descriptor
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

struct DescriptorHeap
{
	ComPtr<ID3D12DescriptorHeap> heap;
	UINT descriptorSize;
	UINT numDescriptorsInHeap;
	UINT numDescriptorsAllocated;

	Descriptor alloc()
	{
		return (*this)[numDescriptorsAllocated++];
	}
	Descriptor operator[](UINT index) const
	{
		assert(index < numDescriptorsInHeap);
		return {
			D3D12_CPU_DESCRIPTOR_HANDLE{heap->GetCPUDescriptorHandleForHeapStart().ptr + index * descriptorSize},
			D3D12_GPU_DESCRIPTOR_HANDLE{heap->GetGPUDescriptorHandleForHeapStart().ptr + index * descriptorSize}
		};
	}
};

struct DescriptorHeapMark
{
	DescriptorHeapMark(DescriptorHeap& heap)
		: heap(heap)
		, mark(heap.numDescriptorsAllocated)
	{}
	~DescriptorHeapMark()
	{
		heap.numDescriptorsAllocated = mark;
	}
	DescriptorHeap& heap;
	const UINT mark;
};

struct StagingBuffer
{
	ComPtr<ID3D12Resource> buffer;
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
	UINT firstSubresource;
	UINT numSubresources;
};

struct UploadBuffer
{
	ComPtr<ID3D12Resource> buffer;
	UINT capacity;
	UINT cursor;
	uint8_t* cpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
};

struct UploadBufferRegion
{
	void* cpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	UINT size;
};

struct MeshBuffer
{
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;
	UINT numElements;
};

struct FrameBuffer
{
	ComPtr<ID3D12Resource> colorTexture;
	ComPtr<ID3D12Resource> depthStencilTexture;
	Descriptor rtv;
	Descriptor dsv;
	Descriptor srv;
	UINT width, height;
	UINT samples;
};

struct ConstantBufferView
{
	UploadBufferRegion data;
	Descriptor cbv;

	template<typename T> T* as() const
	{
		return reinterpret_cast<T*>(data.cpuAddress);
	}
};

struct Texture
{
	ComPtr<ID3D12Resource> texture;
	Descriptor srv;
	Descriptor uav;
	UINT width, height;
	UINT levels;
};

namespace filament
{
	namespace backend
	{
		struct SwapChainBuffer
		{
			ComPtr<ID3D12Resource> buffer;
			std::unique_ptr<MD3D12RenderTargetView> RTV;
			//Descriptor rtv;
		};

		struct D3D12SwapChain;
		struct D3D12RenderTarget;
		class MD3D12DescriptorCache;

		class D3D12Context
		{
		public:
			D3D12Context();
			~D3D12Context();
			ComPtr<IDXGIAdapter1> getAdapter(const ComPtr<IDXGIFactory4>& factory);
			DescriptorHeap createDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc) const;
			//FrameBuffer createFrameBuffer(UINT width, UINT height, UINT samples, DXGI_FORMAT colorFormat, DXGI_FORMAT depthstencilFormat);
			void executeCommandList(bool reset=true) const;
			void waitForGPU() const;
			void SetState(const backend::PipelineState& ps, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
			DXGI_FORMAT GetTextureFormat(TextureFormat format);
			DXGI_FORMAT GetTextureViewFormat(TextureFormat format);
			DXGI_FORMAT GetDepthViewFormat(TextureFormat format);
			ComPtr<ID3D12RootSignature> createRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const;
			ComPtr<ID3DBlob> compileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile);
			//UploadBuffer createUploadBuffer(UINT capacity) const;
			//UploadBufferRegion allocFromUploadBuffer(UploadBuffer& buffer, UINT size, int align) const;
			void SetVertexBuffer(const MD3D12VertexBufferRef& VertexBuffer, UINT Offset, UINT Stride, UINT Size);

			void SetIndexBuffer(const MD3D12IndexBufferRef& IndexBuffer, UINT Offset, DXGI_FORMAT Format, UINT Size);

			void TransitionResource(MD3D12Resource* Resource, D3D12_RESOURCE_STATES StateAfter);

			void CopyResource(MD3D12Resource* DstResource, MD3D12Resource* SrcResource);

			void CopyBufferRegion(MD3D12Resource* DstResource, UINT64 DstOffset, MD3D12Resource* SrcResource, UINT64 SrcOffset, UINT64 Size);

			void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Dst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* Src, const D3D12_BOX* SrcBox);

			MD3D12UploadBufferAllocator* GetUploadBufferAllocator() { return UploadBufferAllocator.get(); }

			MD3D12DefaultBufferAllocator* GetDefaultBufferAllocator() { return DefaultBufferAllocator.get(); }

			MD3D3TextureResourceAllocator* GetTextureResourceAllocator() { return TextureResourceAllocator.get(); }

			MD3D12HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

			MD3D12DescriptorCache* GetDescriptorCache() { return DescriptorCache.get(); }

			struct UniformBufferBinding
			{
				MD3D12ConstantBufferRef ConstantBufferRef;
				//UploadBufferRegion updata;
				//Descriptor cbv;
				size_t offset = 0;
				size_t size = 0;
			};
			
			struct SamplerGroupBinding
			{
				SamplerGroupHandle sampler_group;
			};

			struct RenderState
			{
				D3D12_RASTERIZER_DESC raster;
				D3D12_BLEND_DESC blend;
				D3D12_DEPTH_STENCIL_DESC depth;
			};

			struct {
				ComPtr<ID3D12RootSignature> rootSignature;
				ComPtr<ID3D12PipelineState> linearTexturePipelineState;
				ComPtr<ID3D12PipelineState> gammaTexturePipelineState;
				ComPtr<ID3D12PipelineState> arrayTexturePipelineState;
			} m_mipmapGeneration;

		public:
			ComPtr<IDXGIFactory4> m_dxgiFactory;
			ComPtr<ID3D12Device> m_device;
			ComPtr<ID3D12CommandQueue> m_commandQueue;
			ComPtr<IDXGISwapChain3> m_swapChain;
			ComPtr<ID3D12GraphicsCommandList> m_commandList;
			static const UINT NumFrames = 2;
			ComPtr<ID3D12CommandAllocator> m_commandAllocators[NumFrames];
			FrameBuffer m_framebuffers[NumFrames];
			FrameBuffer m_resolveFramebuffers[NumFrames];
			ConstantBufferView m_transformCBVs[NumFrames];
			ConstantBufferView m_shadingCBVs[NumFrames];

			UINT m_frameIndex = 0;
			ComPtr<ID3D12Fence> m_fence;
			HANDLE m_fenceCompletionEvent;
			mutable UINT64 m_fenceValues[NumFrames] = {};

			int maxSamples = 16;

			D3D_ROOT_SIGNATURE_VERSION m_rootSignatureVersion;
			//////////////////////////////////////////////////////////////////////////
			D3D12SwapChain* current_swap_chain = nullptr;
			D3D12RenderTarget* current_render_target = nullptr;
			RenderPassFlags current_render_pass_flags;
			std::array<UniformBufferBinding, CONFIG_UNIFORM_BINDING_COUNT> uniform_buffer_bindings;
			std::array<SamplerGroupBinding, CONFIG_SAMPLER_BINDING_COUNT> sampler_group_binding;
			std::unordered_map<uint32_t, RenderState> rasterizer_states;

		private:
			std::unique_ptr<MD3D12UploadBufferAllocator> UploadBufferAllocator = nullptr;

			std::unique_ptr<MD3D12DefaultBufferAllocator> DefaultBufferAllocator = nullptr;

			std::unique_ptr<MD3D3TextureResourceAllocator> TextureResourceAllocator = nullptr;

			std::unique_ptr<MD3D12HeapSlotAllocator> RTVHeapSlotAllocator = nullptr;

			std::unique_ptr<MD3D12HeapSlotAllocator> DSVHeapSlotAllocator = nullptr;

			std::unique_ptr<MD3D12HeapSlotAllocator> SRVHeapSlotAllocator = nullptr;

			std::unique_ptr<MD3D12DescriptorCache> DescriptorCache = nullptr;
		};
	}
}
