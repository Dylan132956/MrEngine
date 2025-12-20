#pragma once

#include "Component.h"
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <utils/compiler.h>
#include <utils/debug.h>
#include <utils/bitset.h>
#include <utils/Slice.h>
#include "Allocators.h"
#include "private/backend/DriverApi.h"
#include <backend/DriverEnums.h>
#include <list>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <utility>
#include <filament/Viewport.h>
#include <list>

namespace moonriver {

class Light;
class Camera;

class Froxel {
public:
    enum Planes {
        LEFT, RIGHT, BOTTOM, TOP, NEAR, FAR
    };
    filament::math::float4 planes[6];
};

struct FroxelConfigurationInfo {
	uint16_t width;
	uint16_t height;
	uint16_t depth;
	uint32_t viewportWidth;
	uint32_t viewportHeight;
	filament::math::uint2 froxelDimension;
	float zLightFar;
	float linearizer;
	filament::math::mat4f p;
	filament::math::float4 clipTransform;
};

constexpr size_t CONFIG_MAX_LIGHT_COUNT = 255;

class Froxelizer {
public:
    explicit Froxelizer();
    ~Froxelizer();

    void terminate(filament::backend::DriverApi& driverApi) noexcept;

    // GPU buffer containing records
    const filament::backend::UniformBufferHandle& getRecordBuffer() const noexcept {
        return mRecordsBuffer;
    }

    // GPU buffer containing froxels
    const filament::backend::UniformBufferHandle& getFroxelBuffer() const noexcept {
        return mFroxelsBuffer;
    }

    void setOptions(float zLightNear, float zLightFar) noexcept;

    /*
     * Allocate per-frame data structures for froxelization.
     */
    bool prepare(
            filament::backend::DriverApi& driverApi, filament::RootArenaScope& rootArenaScope,
            const filament::Viewport& viewport,
            const filament::math::mat4f& projection, float projectionNear, float projectionFar) noexcept;

    Froxel getFroxelAt(size_t x, size_t y, size_t z) const noexcept;
    size_t getFroxelCountX() const noexcept { return mFroxelCountX; }
    size_t getFroxelCountY() const noexcept { return mFroxelCountY; }
    size_t getFroxelCountZ() const noexcept { return mFroxelCountZ; }
    size_t getFroxelCount() const noexcept { return mFroxelCount; }

	float getLightFar() const noexcept { return mZLightFar; }
	// Update Records and Froxels texture with lights data
	void froxelizeLights(filament::math::mat4f const& UTILS_RESTRICT viewMatrix,
		const std::vector<Light*>& lights) noexcept;

	void froxelizeLoop(const filament::math::mat4f& UTILS_RESTRICT viewMatrix,
		const std::vector<Light*>& lights) const noexcept;

	static size_t getFroxelBufferByteCount(filament::backend::DriverApi& driverApi) noexcept;

	static size_t getFroxelRecordBufferByteCount(filament::backend::DriverApi& driverApi) noexcept;

    // send froxel data to GPU
    void commit(filament::backend::DriverApi& driverApi);

    // Froxel entry structure for GPU
    struct FroxelEntry {
        FroxelEntry(uint16_t const offset, uint8_t const count) noexcept
            : u32((offset << 16) | count) { }

        uint8_t count() const noexcept { return u32 & 0xFFu; }
        uint16_t offset() const noexcept { return u32 >> 16u; }
        uint32_t u32 = 0;
    };
    //static_assert(sizeof(FroxelEntry) == 4u);

    // Record buffer type
    using RecordBufferType = uint8_t;

	utils::Slice<const FroxelEntry> getFroxelBufferUser() const { return mFroxelBufferUser; }
	utils::Slice<const RecordBufferType> getRecordBufferUser() const { return mRecordBufferUser; }

    using LightGroupType = uint32_t;
private:
    struct FroxelThreadData;

	inline void setViewport(filament::Viewport const& viewport) noexcept;
	inline void setProjection(const filament::math::mat4f& projection, float near, float far) noexcept;

	size_t getFroxelBufferEntryCount() const noexcept {
		// We guarantee that mFroxelBufferEntryCount is a multiple of 16. With this knowledge
		// the compiler can do a much better job at vectorizing. For similar reasons, it's
		// important to keep mFroxelBufferEntryCount an uint32_t (as opposed to a size_t).
		//assert_invariant((mFroxelBufferEntryCount & 0xF) == 0);
		UTILS_ASSUME((mFroxelBufferEntryCount & 0xF) == 0);
		UTILS_ASSUME(mFroxelBufferEntryCount >= 16);
		return mFroxelBufferEntryCount;
	}

	struct LightRecord {
		using bitset = utils::bitset<uint64_t, (CONFIG_MAX_LIGHT_COUNT + 63) / 64>;
		bitset lights;
	};

	struct LightParams {
		filament::math::float3 position;
		float cosSqr;
		filament::math::float3 axis;
		// this must be initialized to indicate this is a point light
		float invSin = std::numeric_limits<float>::infinity();
		// radius is not used in the hot loop, so leave it at the end
		float radius;
	};

    void computeFroxelLayout(
            filament::math::uint2* dim, uint16_t* countX, uint16_t* countY, uint16_t* countZ,
            size_t const froxelBufferEntryCount, filament::Viewport const& viewport) noexcept;

    size_t findSliceZ(float const viewSpaceZ) const noexcept;
    std::pair<size_t, size_t> clipToIndices(filament::math::float2 const& clip) const noexcept;
	static size_t getFroxelIndex(size_t const ix, size_t const iy, size_t const iz,
		size_t const froxelCountX, size_t const froxelCountY) noexcept {
		return ix + (iy * froxelCountX) + (iz * froxelCountX * froxelCountY);
	}

	size_t getFroxelIndex(size_t const ix, size_t const iy, size_t const iz) const noexcept {
		return getFroxelIndex(ix, iy, iz, mFroxelCountX, mFroxelCountY);
	}
    
	static void updateBoundingSpheres(
		filament::math::float4* UTILS_RESTRICT boundingSpheres,
		size_t froxelCountX, size_t froxelCountY, size_t froxelCountZ,
		filament::math::float4 const* UTILS_RESTRICT planesX,
		filament::math::float4 const* UTILS_RESTRICT planesY,
		float const* UTILS_RESTRICT planesZ) noexcept;

    bool update() noexcept;

	void froxelizeAssignRecordsCompress() noexcept;
	void froxelizePointAndSpotLight(FroxelThreadData& froxelThread, size_t bit,
		filament::math::mat4f const& projection, const LightParams& light) const noexcept;

	filament::LinearAllocatorArena mArena;
	// 4096 froxels fits in a 16KiB buffer, the minimum guaranteed in GLES 3.x and Vulkan 1.1
	uint32_t mFroxelBufferEntryCount = 4096;

	// 16384 entries is our minimum with a 16KiB buffer
	uint32_t mFroxelRecordBufferEntryCount = 16384;

	// allocations in the private froxel arena
	float* mDistancesZ = nullptr;
	filament::math::float4* mPlanesX = nullptr;
	filament::math::float4* mPlanesY = nullptr;
	filament::math::float4* mBoundingSpheres = nullptr;           // 64 KiB w/ 4096 froxels

	// allocations in the per frame arena
	//        max |  real | size
	//       8192 |  4096 | 512 KiB
	//       8192 |  8192 | 768 KiB
	//      65536 | 65536 | 6.0 MiB
	utils::Slice<LightRecord> mLightRecords;            // 256 KiB w/  256 lights and 4096 froxels
	utils::Slice<FroxelThreadData> mFroxelShardedData;  // 256 KiB w/  256 lights and 8192 max froxels

	// allocations in the command stream
	utils::Slice<FroxelEntry> mFroxelBufferUser;        //  16 KiB w/ 4096 froxels
	utils::Slice<RecordBufferType> mRecordBufferUser;   //  16 KiB to 64 KiB

	uint16_t mFroxelCountX = 0;
	uint16_t mFroxelCountY = 0;
	uint16_t mFroxelCountZ = 0;
	uint32_t mFroxelCount = 0;
	filament::math::uint2 mFroxelDimension = {};
    filament::math::float4 mClipTransform = { 1, 1, 0, 0 };

    filament::math::mat4f mProjection;
    filament::math::float2 mLinearizer{};
	float mClipToFroxelX = 0.0f;
	float mClipToFroxelY = 0.0f;
    filament::backend::UniformBufferHandle mRecordsBuffer;
    filament::backend::UniformBufferHandle mFroxelsBuffer;

	// needed for update()
    filament::Viewport mViewport;
    filament::math::float4 mParamsZ = {};
    filament::math::uint3 mParamsF = {};
	float mNear = 0.0f;        // camera near
	float mFar = 0.0f;         // culling camera far
	float mZLightNear;
	float mZLightFar;
	float mUserZLightNear;
	float mUserZLightFar;

	// track if we need to update our internal state before froxelizing
	uint8_t mDirtyFlags = 0;
	enum {
		VIEWPORT_CHANGED = 0x01,
		PROJECTION_CHANGED = 0x02,
		OPTIONS_CHANGED = 0x04
	};
	FroxelConfigurationInfo mFroxelConfigurationInfo{};
};

} // namespace moonriver
