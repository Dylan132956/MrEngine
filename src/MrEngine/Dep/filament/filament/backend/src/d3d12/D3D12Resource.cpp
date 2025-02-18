#include "D3D12Resource.h"
#include "D3D12MemoryAllocator.h"

namespace filament
{
	namespace backend
	{
		using namespace Microsoft::WRL;

		MD3D12Resource::MD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState)
			:D3DResource(InD3DResource), CurrentState(InitState)
		{
			if (D3DResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				GPUVirtualAddress = D3DResource->GetGPUVirtualAddress();
			}
			else
			{
				// GetGPUVirtualAddress() returns NULL for non-buffer resources.
			}
		}

		void MD3D12Resource::Map()
		{
			ThrowIfFailed(D3DResource->Map(0, nullptr, &MappedBaseAddress));
		}

		MD3D12ResourceLocation::MD3D12ResourceLocation()
		{

		}

		MD3D12ResourceLocation::~MD3D12ResourceLocation()
		{
			ReleaseResource();
		}

		void MD3D12ResourceLocation::ReleaseResource()
		{
			switch (ResourceLocationType)
			{
			case MD3D12ResourceLocation::EResourceLocationType::StandAlone:
			{
				delete UnderlyingResource;

				break;
			}
			case MD3D12ResourceLocation::EResourceLocationType::SubAllocation:
			{
				if (Allocator)
				{
					Allocator->Deallocate(*this);
				}

				break;
			}

			default:
				break;
			}
		}
	}
}