
#include "PlatformD3D11.h"
#include "D3D11DriverFactory.h"

namespace filament
{
	namespace backend
	{
		D3D11Platform::~D3D11Platform() = default;
	}

	using namespace backend;

	Driver* PlatformD3D11::createDriver(void* sharedContext) noexcept
	{
		return D3D11DriverFactory::create(this);
	}

	PlatformD3D11::~PlatformD3D11() = default;
}
