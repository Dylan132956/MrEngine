
#include "PlatformD3D12.h"
#include "D3D12DriverFactory.h"

namespace filament
{
	namespace backend
	{
		D3D12Platform::~D3D12Platform() = default;
	}

	using namespace backend;

	Driver* PlatformD3D12::createDriver(void* sharedContext) noexcept
	{
		return D3D12DriverFactory::create(this);
	}

	PlatformD3D12::~PlatformD3D12() = default;
}
