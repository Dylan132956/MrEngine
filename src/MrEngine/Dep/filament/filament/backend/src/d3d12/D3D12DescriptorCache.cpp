#include "D3D12Context.h"
#include "D3D12DescriptorCache.h"

namespace filament
{
	namespace backend
	{

		MD3D12DescriptorCache::MD3D12DescriptorCache(D3D12Context* InContext)
			:m_context(InContext)
		{
			CreateCacheCbvSrvUavDescriptorHeap();

			CreateCacheRtvDescriptorHeap();
		}

		MD3D12DescriptorCache::~MD3D12DescriptorCache()
		{

		}

		void MD3D12DescriptorCache::CreateCacheCbvSrvUavDescriptorHeap()
		{
			// Create the descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {};
			SrvHeapDesc.NumDescriptors = MaxCbvSrvUavDescripotrCount;
			SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(m_context->m_device->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(&CacheCbvSrvUavDescriptorHeap)));
			SetDebugName(CacheCbvSrvUavDescriptorHeap.Get(), L"MD3D12DescriptorCache CacheCbvSrvUavDescriptorHeap");

			CbvSrvUavDescriptorSize = m_context->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}


		CD3DX12_GPU_DESCRIPTOR_HANDLE MD3D12DescriptorCache::AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& SrcDescriptors)
		{
			// Append to heap
			uint32_t SlotsNeeded = (uint32_t)SrcDescriptors.size();
			assert(CbvSrvUavDescriptorOffset + SlotsNeeded < MaxCbvSrvUavDescripotrCount);

			auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), CbvSrvUavDescriptorOffset, CbvSrvUavDescriptorSize);
			m_context->m_device->CopyDescriptors(1, &CpuDescriptorHandle, &SlotsNeeded, SlotsNeeded, SrcDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			// Get GpuDescriptorHandle
			auto GpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), CbvSrvUavDescriptorOffset, CbvSrvUavDescriptorSize);

			// Increase descriptor offset
			CbvSrvUavDescriptorOffset += SlotsNeeded;

			return GpuDescriptorHandle;
		}

		void MD3D12DescriptorCache::ResetCacheCbvSrvUavDescriptorHeap()
		{
			CbvSrvUavDescriptorOffset = 0;
		}

		void MD3D12DescriptorCache::CreateCacheRtvDescriptorHeap()
		{
			// Create the descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
			RtvHeapDesc.NumDescriptors = MaxRtvDescriptorCount;
			RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			ThrowIfFailed(m_context->m_device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&CacheRtvDescriptorHeap)));
			SetDebugName(CacheRtvDescriptorHeap.Get(), L"MD3D12DescriptorCache CacheRtvDescriptorHeap");

			RtvDescriptorSize = m_context->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		void MD3D12DescriptorCache::AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& OutGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& OutCpuHandle)
		{
			// Append to heap
			uint32_t SlotsNeeded = (uint32_t)RtvDescriptors.size();
			assert(RtvDescriptorOffset + SlotsNeeded < MaxRtvDescriptorCount);

			auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);
			m_context->m_device->CopyDescriptors(1, &CpuDescriptorHandle, &SlotsNeeded, SlotsNeeded, RtvDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			OutGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);

			OutCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);

			// Increase descriptor offset
			RtvDescriptorOffset += SlotsNeeded;
		}

		void MD3D12DescriptorCache::ResetCacheRtvDescriptorHeap()
		{
			RtvDescriptorOffset = 0;
		}

		void MD3D12DescriptorCache::Reset()
		{
			ResetCacheCbvSrvUavDescriptorHeap();

			ResetCacheRtvDescriptorHeap();
		}

	}
}