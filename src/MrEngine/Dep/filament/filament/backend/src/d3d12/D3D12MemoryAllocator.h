#pragma once

#include "D3D12Resource.h"
#include <stdint.h>
#include <set>
#include <memory>

#define DEFAULT_POOL_SIZE (512 * 1024 * 512)

#define DEFAULT_RESOURCE_ALIGNMENT 4
#define UPLOAD_RESOURCE_ALIGNMENT 256
namespace filament
{
	namespace backend
	{
		class MD3D12BuddyAllocator
		{
		public:
			enum class EAllocationStrategy
			{
				PlacedResource,
				ManualSubAllocation
			};

			struct TAllocatorInitData
			{
				EAllocationStrategy AllocationStrategy;

				D3D12_HEAP_TYPE HeapType;

				D3D12_HEAP_FLAGS HeapFlags = D3D12_HEAP_FLAG_NONE;  // Only for PlacedResource

				D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;  // Only for ManualSubAllocation
			};

		public:
			MD3D12BuddyAllocator(ID3D12Device* InDevice, const TAllocatorInitData& InInitData);

			~MD3D12BuddyAllocator();

			bool AllocResource(uint32_t Size, uint32_t Alignment, MD3D12ResourceLocation& ResourceLocation);

			void Deallocate(MD3D12ResourceLocation& ResourceLocation);

			void CleanUpAllocations();

			ID3D12Heap* GetBackingHeap() { return BackingHeap; }

			EAllocationStrategy GetAllocationStrategy() { return InitData.AllocationStrategy; }

		private:
			void Initialize();

			uint32_t AllocateBlock(uint32_t Order);

			uint32_t GetSizeToAllocate(uint32_t Size, uint32_t Alignment);

			bool CanAllocate(uint32_t SizeToAllocate);

			uint32_t SizeToUnitSize(uint32_t Size) const
			{
				return (Size + (MinBlockSize - 1)) / MinBlockSize;
			}

			uint32_t UnitSizeToOrder(uint32_t Size) const
			{
				unsigned long Result;
				_BitScanReverse(&Result, Size + Size - 1); // ceil(log2(size))
				return Result;
			}

			uint32_t OrderToUnitSize(uint32_t Order) const
			{
				return ((uint32_t)1) << Order;
			}

			void DeallocateInternal(const MD3D12BuddyBlockData& Block);

			void DeallocateBlock(uint32_t Offset, uint32_t Order);

			uint32_t GetBuddyOffset(const uint32_t& Offset, const uint32_t& Size)
			{
				return Offset ^ Size;
			}

			uint32_t GetAllocOffsetInBytes(uint32_t Offset) const { return Offset * MinBlockSize; }

		private:
			TAllocatorInitData InitData;

			const uint32_t MinBlockSize = 256;

			uint32_t MaxOrder;

			uint32_t TotalAllocSize = 0;

			std::vector<std::set<uint32_t>> FreeBlocks;

			std::vector<MD3D12BuddyBlockData> DeferredDeletionQueue;

			ID3D12Device* D3DDevice;

			MD3D12Resource* BackingResource = nullptr;

			ID3D12Heap* BackingHeap = nullptr;
		};

		class MD3D12MultiBuddyAllocator
		{
		public:
			MD3D12MultiBuddyAllocator(ID3D12Device* InDevice, const MD3D12BuddyAllocator::TAllocatorInitData& InInitData);

			~MD3D12MultiBuddyAllocator();

			bool AllocResource(uint32_t Size, uint32_t Alignment, MD3D12ResourceLocation& ResourceLocation);

			void CleanUpAllocations();

		private:
			std::vector<std::shared_ptr<MD3D12BuddyAllocator>> Allocators;

			ID3D12Device* Device;

			MD3D12BuddyAllocator::TAllocatorInitData InitData;
		};

		class MD3D12UploadBufferAllocator
		{
		public:
			MD3D12UploadBufferAllocator(ID3D12Device* InDevice);

			void* AllocUploadResource(uint32_t Size, uint32_t Alignment, MD3D12ResourceLocation& ResourceLocation);

			void CleanUpAllocations();

		private:
			std::unique_ptr<MD3D12MultiBuddyAllocator> Allocator = nullptr;

			ID3D12Device* D3DDevice = nullptr;
		};

		class MD3D12DefaultBufferAllocator
		{
		public:
			MD3D12DefaultBufferAllocator(ID3D12Device* InDevice);

			void AllocDefaultResource(const D3D12_RESOURCE_DESC& ResourceDesc, uint32_t Alignment, MD3D12ResourceLocation& ResourceLocation);

			void CleanUpAllocations();

		private:
			std::unique_ptr<MD3D12MultiBuddyAllocator> Allocator = nullptr;

			std::unique_ptr<MD3D12MultiBuddyAllocator> UavAllocator = nullptr;

			ID3D12Device* D3DDevice = nullptr;
		};

		class MD3D3TextureResourceAllocator
		{
		public:
			MD3D3TextureResourceAllocator(ID3D12Device* InDevice);

			void AllocTextureResource(const D3D12_RESOURCE_STATES& ResourceState, const D3D12_RESOURCE_DESC& ResourceDesc, MD3D12ResourceLocation& ResourceLocation);

			void CleanUpAllocations();

		private:
			std::unique_ptr<MD3D12MultiBuddyAllocator> Allocator = nullptr;

			ID3D12Device* D3DDevice = nullptr;
		};
	}
}