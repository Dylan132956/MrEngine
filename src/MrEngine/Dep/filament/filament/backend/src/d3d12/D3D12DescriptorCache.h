#pragma once

#include "D3D12Utils.h"
#include "D3D12MemoryAllocator.h"

namespace filament
{
	namespace backend
	{
		class D3D12Context;

		class MD3D12DescriptorCache
		{
		public:
			MD3D12DescriptorCache(D3D12Context* InContext);

			~MD3D12DescriptorCache();

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCacheCbvSrvUavDescriptorHeap() { return CacheCbvSrvUavDescriptorHeap; }

			CD3DX12_GPU_DESCRIPTOR_HANDLE AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& SrcDescriptors);

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCacheRtvDescriptorHeap() { return CacheRtvDescriptorHeap; }

			void AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& OutGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& OutCpuHandle);

			// Switch to FrameSlot's region and discard what that slot used last time.
			// Caller must have waited for the GPU work that last used this slot.
			void Reset(uint32_t FrameSlot);

		private:
			void CreateCacheCbvSrvUavDescriptorHeap();

			void CreateCacheRtvDescriptorHeap();

			void ResetCacheCbvSrvUavDescriptorHeap(uint32_t FrameSlot);

			void ResetCacheRtvDescriptorHeap(uint32_t FrameSlot);

		private:
			D3D12Context* m_context;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheCbvSrvUavDescriptorHeap = nullptr;

			UINT CbvSrvUavDescriptorSize;

			// Split evenly across kD3D12FrameSlots so in-flight frames keep their descriptors.
			static const int MaxCbvSrvUavDescripotrCount = 4096;

			uint32_t CbvSrvUavDescriptorOffset = 0;

			uint32_t CbvSrvUavRegionEnd = MaxCbvSrvUavDescripotrCount / kD3D12FrameSlots;

		private:
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheRtvDescriptorHeap = nullptr;

			UINT RtvDescriptorSize;

			static const int MaxRtvDescriptorCount = 1024;

			uint32_t RtvDescriptorOffset = 0;

			uint32_t RtvRegionEnd = MaxRtvDescriptorCount / kD3D12FrameSlots;
		};
	}
}