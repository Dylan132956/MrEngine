#pragma once

#include "D3D12Utils.h"

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

			void Reset();

		private:
			void CreateCacheCbvSrvUavDescriptorHeap();

			void CreateCacheRtvDescriptorHeap();

			void ResetCacheCbvSrvUavDescriptorHeap();

			void ResetCacheRtvDescriptorHeap();

		private:
			D3D12Context* m_context;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheCbvSrvUavDescriptorHeap = nullptr;

			UINT CbvSrvUavDescriptorSize;

			static const int MaxCbvSrvUavDescripotrCount = 2048;

			uint32_t CbvSrvUavDescriptorOffset = 0;

		private:
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheRtvDescriptorHeap = nullptr;

			UINT RtvDescriptorSize;

			static const int MaxRtvDescriptorCount = 1024;

			uint32_t RtvDescriptorOffset = 0;
		};
	}
}