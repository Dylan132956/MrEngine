#include "Froxelizer.h"
#include "Light.h"
#include "Camera.h"
#include "Transform.h"
#include "private/backend/DriverApi.h"
#include <utils/architecture.h>
#include "Intersections.h"
#include <filament/Box.h>
#include <utils/BinaryTreeArray.h>
#include "math/fast.h"
#include "math/scalar.h"
#include "utils/JobSystem.h"
#include "utils/Log.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>
#include <cassert>
#include "Engine.h"
#include "Intersections.h"

using namespace moonriver;

// Constants for froxelization
constexpr size_t FROXEL_BUFFER_MAX_ENTRY_COUNT = 8192;
constexpr size_t FROXEL_SLICE_COUNT = 16;
constexpr float FROXEL_FIRST_SLICE_DEPTH_DEFAULT = 5.0f;      // 5m
constexpr float FROXEL_LAST_SLICE_DISTANCE_DEFAULT = 100.0f;  // 100m

// Buffer needed for Froxelizer internal data structures (~256 KiB)
constexpr size_t PER_FROXELDATA_ARENA_SIZE = sizeof(Vector4) *
                               (FROXEL_BUFFER_MAX_ENTRY_COUNT +
	                            FROXEL_BUFFER_MAX_ENTRY_COUNT + 3 +
	                            FROXEL_SLICE_COUNT / 4 + 1);

// Number of lights processed by one group (e.g. 32)
static constexpr size_t LIGHT_PER_GROUP = sizeof(Froxelizer::LightGroupType) * 8;

constexpr size_t CONFIG_MAX_LIGHT_INDEX = CONFIG_MAX_LIGHT_COUNT - 1;
// Number of groups (i.e. jobs) to use for froxelization (e.g. 8)
static constexpr size_t GROUP_COUNT =
(CONFIG_MAX_LIGHT_COUNT + LIGHT_PER_GROUP - 1) / LIGHT_PER_GROUP;

// This depends on the maximum number of lights (currently 256)
static_assert(CONFIG_MAX_LIGHT_INDEX <= std::numeric_limits<Froxelizer::RecordBufferType>::max(),
	"can't have more than 256 lights");

struct Froxelizer::FroxelThreadData :
	public std::array<LightGroupType, FROXEL_BUFFER_MAX_ENTRY_COUNT> {
};

// Returns false if the two matrices are different. May return false if they're the
// same, with some elements only differing by +0 or -0. Behaviour is undefined with NaNs.
static bool fuzzyEqual(filament::math::mat4f const& UTILS_RESTRICT l, filament::math::mat4f const& UTILS_RESTRICT r) noexcept {
	auto const li = reinterpret_cast<uint32_t const*>(reinterpret_cast<char const*>(&l));
	auto const ri = reinterpret_cast<uint32_t const*>(reinterpret_cast<char const*>(&r));
	uint32_t result = 0;
	for (size_t i = 0; i < sizeof(filament::math::mat4f) / sizeof(uint32_t); i++) {
		// clang fully vectorizes this
		result |= li[i] ^ ri[i];
	}
	return result == 0;
}

size_t Froxelizer::getFroxelBufferByteCount(filament::backend::DriverApi& driverApi) noexcept {
	// Make sure that targetSize is 16-byte aligned so that it'll fit properly into an array of
	// uvec4.
	size_t const targetSize = (65536 / 16) * 16;
	return std::min(FROXEL_BUFFER_MAX_ENTRY_COUNT * sizeof(FroxelEntry), targetSize);
}

size_t Froxelizer::getFroxelRecordBufferByteCount(filament::backend::DriverApi& driverApi) noexcept {
	// Make sure that targetSize is 16-byte aligned so that it'll fit properly into an array of
	// uvec4. The maximum size is 64K entries, because we're using 16 bits indices.
	size_t const targetSize = (65536 / 16) * 16;
	return std::min(size_t(std::numeric_limits<uint16_t>::max()), targetSize);
}

Froxelizer::Froxelizer()
    : mArena("froxel", PER_FROXELDATA_ARENA_SIZE),
      mZLightNear(FROXEL_FIRST_SLICE_DEPTH_DEFAULT),
      mZLightFar(FROXEL_LAST_SLICE_DISTANCE_DEFAULT),
      mUserZLightNear(FROXEL_FIRST_SLICE_DEPTH_DEFAULT),
      mUserZLightFar(FROXEL_LAST_SLICE_DISTANCE_DEFAULT)
{
	static_assert(std::is_same_v<RecordBufferType, uint8_t>,
		"Record Buffer must use bytes");

    auto& driver = Engine::Instance()->GetDriverApi();

	size_t const froxelBufferByteCount = getFroxelBufferByteCount(driver);
	mFroxelBufferEntryCount = froxelBufferByteCount / sizeof(FroxelEntry);
	mFroxelBufferEntryCount &= ~0xF; // make sure it's a multiple of 16 (helps vectorizing)
	assert_invariant(mFroxelBufferEntryCount >= 16); // that's also needed elsewhere

	size_t const froxelRecordBufferByteCount = getFroxelRecordBufferByteCount(driver);
	mFroxelRecordBufferEntryCount = froxelRecordBufferByteCount / sizeof(uint8_t);
	assert_invariant(mFroxelRecordBufferEntryCount <= std::numeric_limits<uint16_t>::max());

	mRecordsBuffer = driver.createUniformBuffer(froxelRecordBufferByteCount, filament::backend::BufferUsage::DYNAMIC);

    mFroxelsBuffer = driver.createUniformBuffer(froxelBufferByteCount, filament::backend::BufferUsage::DYNAMIC);
}

Froxelizer::~Froxelizer() {
    // Ensure terminate() was called
}

void Froxelizer::terminate(filament::backend::DriverApi& driverApi) noexcept {
    if (mRecordsBuffer) {
        driverApi.destroyUniformBuffer(mRecordsBuffer);
        mRecordsBuffer.clear();
    }
    if (mFroxelsBuffer) {
        driverApi.destroyUniformBuffer(mFroxelsBuffer);
        mFroxelsBuffer.clear();
    }

    // Reset the arena to release all internal buffers
    mArena.reset();
}

void Froxelizer::setOptions(float zLightNear, float zLightFar) noexcept {
    if (mUserZLightNear != zLightNear || mUserZLightFar != zLightFar) {
        mUserZLightNear = zLightNear;
        mUserZLightFar = zLightFar;
        mDirtyFlags |= OPTIONS_CHANGED;
    }
}
void Froxelizer::setViewport(filament::Viewport const& viewport) noexcept {
	if (UTILS_UNLIKELY(mViewport != viewport)) {
		mViewport = viewport;
		mDirtyFlags |= VIEWPORT_CHANGED;
	}
}

void Froxelizer::setProjection(const filament::math::mat4f& projection,
	float const near, UTILS_UNUSED float const far) noexcept {
	if (UTILS_UNLIKELY(!fuzzyEqual(mProjection, projection) || mNear != near || mFar != far)) {
		mProjection = projection;
		mNear = near;
		mFar = far;
		mDirtyFlags |= PROJECTION_CHANGED;
	}
}

bool Froxelizer::prepare(
        filament::backend::DriverApi& driverApi, filament::RootArenaScope& rootArenaScope,
        const filament::Viewport& viewport,
        const filament::math::mat4f& projection, float projectionNear, float projectionFar) noexcept {
    assert(projectionFar > projectionNear);
    assert(projectionNear > 0);

    // Update viewport and projection
	setViewport(viewport);
	setProjection(projection, projectionNear, projectionFar);

    bool uniformsNeedUpdating = false;
    if (mDirtyFlags) {
        uniformsNeedUpdating = update();
    }

    /*
     * Allocations that need to persist until the driver consumes them are done from
     * the command stream.
     */

     // froxel buffer (16 KiB with 4096 froxels)
    mFroxelBufferUser.set(
        driverApi.allocatePod<FroxelEntry>(mFroxelBufferEntryCount),
        mFroxelBufferEntryCount);

    // record buffer (64 KiB max)
    mRecordBufferUser.set(
        driverApi.allocatePod<RecordBufferType>(mFroxelRecordBufferEntryCount),
        mFroxelRecordBufferEntryCount);

    /*
     * Temporary allocations for processing all froxel data
     */

     // light records per froxel (~256 KiB with 4096 froxels)
    mLightRecords.set(
        rootArenaScope.allocate<LightRecord>(getFroxelBufferEntryCount(), utils::CACHELINE_SIZE),
        getFroxelBufferEntryCount());

    // froxel thread data (~256KiB with 8192 max froxels and 256 lights)
    mFroxelShardedData.set(
        rootArenaScope.allocate<FroxelThreadData>(GROUP_COUNT, utils::CACHELINE_SIZE),
        uint32_t(GROUP_COUNT));

    assert_invariant(mFroxelBufferUser.begin());
    assert_invariant(mRecordBufferUser.begin());
    assert_invariant(mLightRecords.begin());
    assert_invariant(mFroxelShardedData.begin());

    // initialize buffers that need to be
    memset(mLightRecords.data(), 0, mLightRecords.sizeInBytes());

    return uniformsNeedUpdating;
}

void Froxelizer::computeFroxelLayout(
        filament::math::uint2* dim, uint16_t* countX, uint16_t* countY, uint16_t* countZ,
        size_t const froxelBufferEntryCount, filament::Viewport const& viewport) noexcept {
	auto roundTo8 = [](uint32_t const v) { return (v + 7u) & ~7u; };

	const uint32_t width = std::max(16u, viewport.width);
	const uint32_t height = std::max(16u, viewport.height);

	// calculate froxel dimension from FROXEL_BUFFER_ENTRY_COUNT_MAX and viewport
	// - Start from the maximum number of froxels we can use in the x-y plane
	constexpr size_t froxelSliceCount = FROXEL_SLICE_COUNT;
	size_t const froxelPlaneCount = froxelBufferEntryCount / froxelSliceCount;
	// - compute the number of square froxels we need in width and height, rounded down
	//   solving: |  froxelCountX * froxelCountY == froxelPlaneCount
	//            |  froxelCountX / froxelCountY == width / height
	size_t froxelCountX = size_t(std::sqrt(froxelPlaneCount * width / height));
	size_t froxelCountY = size_t(std::sqrt(froxelPlaneCount * height / width));
	// - compute the froxels dimensions, rounded up
	size_t const froxelSizeX = (width + froxelCountX - 1) / froxelCountX;
	size_t const froxelSizeY = (height + froxelCountY - 1) / froxelCountY;
	// - and since our froxels must be square, only keep the largest dimension

	//  make sure we're at lease multiple of 8 to improve performance in the shader
	size_t const froxelDimension = roundTo8((roundTo8(froxelSizeX) >= froxelSizeY) ? froxelSizeX : froxelSizeY);

	// Here we recompute the froxel counts which may have changed a little due to the rounding
	// and the squareness requirement of froxels
	froxelCountX = (width + froxelDimension - 1) / froxelDimension;
	froxelCountY = (height + froxelDimension - 1) / froxelDimension;

	assert_invariant(froxelCountX);
	assert_invariant(froxelCountY);
	assert_invariant(froxelCountX * froxelCountY <= froxelPlaneCount);

	*dim = froxelDimension;
	*countX = uint16_t(froxelCountX);
	*countY = uint16_t(froxelCountY);
	*countZ = uint16_t(froxelSliceCount);
}

UTILS_NOINLINE
bool Froxelizer::update() noexcept {
	bool uniformsNeedUpdating = false;

	if (UTILS_UNLIKELY(mDirtyFlags & (OPTIONS_CHANGED | PROJECTION_CHANGED))) {

		// sanitize the user's near/far
		float zLightNear = mUserZLightNear;
		float zLightFar = mUserZLightFar;
		if (zLightFar == zLightNear) {
			zLightNear = mNear;
			zLightFar = mFar;
		}
		if (zLightFar < zLightNear) {
			std::swap(zLightFar, zLightNear);
		}
		if (zLightNear < mNear || zLightNear >= mFar) {
			zLightNear = mNear;
		}
		if (zLightFar > mFar || zLightFar <= mNear) {
			zLightFar = mFar;
		}

		assert_invariant(zLightNear < zLightFar);
		assert_invariant(zLightNear >= mNear && zLightNear <= mFar);
		assert_invariant(zLightFar <= mFar && zLightNear >= mNear);

		zLightNear = std::min(zLightNear, zLightFar);
		if (zLightFar != mZLightFar || zLightNear != mZLightNear) {
			mDirtyFlags |= VIEWPORT_CHANGED;
			mZLightNear = zLightNear;
			mZLightFar = zLightFar;
		}
	}

	if (UTILS_UNLIKELY(mDirtyFlags & VIEWPORT_CHANGED)) {
		filament::Viewport const& viewport = mViewport;

		filament::math::uint2 froxelDimension;
		uint16_t froxelCountX, froxelCountY, froxelCountZ;
		computeFroxelLayout(&froxelDimension, &froxelCountX, &froxelCountY, &froxelCountZ,
			getFroxelBufferEntryCount(), viewport);

		mFroxelDimension = froxelDimension;
		// note: because froxelDimension is a power-of-two and viewport is an integer, mClipFroxel
		// is an exact value (which is not true for 1/mClipToFroxelX, btw)
		mClipToFroxelX = float(viewport.width) / float(2 * froxelDimension.x);
		mClipToFroxelY = float(viewport.height) / float(2 * froxelDimension.y);

		uniformsNeedUpdating = true;

		//DLOG(INFO) << "Froxel: " << viewport.width << "x" << viewport.height << " / "
		//	<< froxelDimension.x << "x" << froxelDimension.y << io::endl
		//	<< "Froxel: " << froxelCountX << "x" << froxelCountY << "x" << froxelCountZ
		//	<< " = " << (froxelCountX * froxelCountY * froxelCountZ) << " ("
		//	<< getFroxelBufferEntryCount() - froxelCountX * froxelCountY * froxelCountZ
		//	<< " lost)";

		mFroxelCountX = froxelCountX;
		mFroxelCountY = froxelCountY;
		mFroxelCountZ = froxelCountZ;
		const uint32_t froxelCount = uint32_t(froxelCountX * froxelCountY * froxelCountZ);
		mFroxelCount = froxelCount;

		if (mDistancesZ) {
			// this is a LinearAllocator arena, use rewind() instead of free (which is a no op).
			mArena.rewind(mDistancesZ);
		}

		mDistancesZ = mArena.alloc<float>(froxelCountZ + 1);
		mPlanesX = mArena.alloc<filament::math::float4>(froxelCountX + 1);
		mPlanesY = mArena.alloc<filament::math::float4>(froxelCountY + 1);
		mBoundingSpheres = mArena.alloc<filament::math::float4>(froxelCount);

		assert_invariant(mDistancesZ);
		assert_invariant(mPlanesX);
		assert_invariant(mPlanesY);
		assert_invariant(mBoundingSpheres);

		mDistancesZ[0] = 0.0f;
		const float zLightNear = mZLightNear;
		const float zLightFar = mZLightFar;
		const float linearizer = std::log2(zLightFar / zLightNear) / float(std::max(1u, mFroxelCountZ - 1u));
		// for a strange reason when, vectorizing this loop, clang does some math in double
		// and generates conversions to float. not worth it for so little iterations.
#if defined(__clang__)
#pragma clang loop vectorize(disable) unroll(disable)
#endif
		for (ssize_t i = 1, n = mFroxelCountZ; i <= n; i++) {
			mDistancesZ[i] = zLightFar * std::exp2(float(i - n) * linearizer);
		}

		// for the inverse-transformation (view-space z to z-slice)
		mLinearizer = { linearizer, 1.0f / linearizer };

		mParamsZ[0] = 0; // updated when camera changes
		mParamsZ[1] = 0; // updated when camera changes
		mParamsZ[2] = 0; // updated when camera changes
		mParamsZ[3] = mFroxelCountZ;
		mParamsF[0] = 1;
		mParamsF[1] = uint32_t(mFroxelCountX);
		mParamsF[2] = uint32_t(mFroxelCountX * mFroxelCountY);
	}

	if (UTILS_UNLIKELY(mDirtyFlags & (PROJECTION_CHANGED | VIEWPORT_CHANGED))) {
		assert_invariant(mDistancesZ);
		assert_invariant(mPlanesX);
		assert_invariant(mPlanesY);
		assert_invariant(mBoundingSpheres);

		// clip-space dimensions
		const float froxelWidthInClipSpace = float(2 * mFroxelDimension.x) / float(mViewport.width);
		const float froxelHeightInClipSpace = float(2 * mFroxelDimension.y) / float(mViewport.height);
		filament::math::float4* const UTILS_RESTRICT planesX = mPlanesX;
		filament::math::float4* const UTILS_RESTRICT planesY = mPlanesY;

		// Planes are transformed by the inverse-transpose of the transform matrix.
		// So to transform a plane in clip-space to view-space, we need to apply
		// the transpose(inverse(viewFromClipMatrix)), i.e.: transpose(projection)
		const filament::math::mat4f trProjection(transpose(mProjection));

		// generate the horizontal planes from their clip-space equation
		for (size_t i = 0, n = mFroxelCountX; i <= n; ++i) {
			float const x = (float(i) * froxelWidthInClipSpace) - 1.0f;
			filament::math::float4 const p = trProjection * filament::math::float4{ -1, 0, 0, x };
			planesX[i] = filament::math::float4{ normalize(p.xyz), 0 };  // p.w is guaranteed to be 0
		}

		// generate the vertical planes from their clip-space equation
		for (size_t i = 0, n = mFroxelCountY; i <= n; ++i) {
			float const y = (float(i) * froxelHeightInClipSpace) - 1.0f;
			filament::math::float4 const p = trProjection * filament::math::float4{ 0, 1, 0, -y };
			planesY[i] = filament::math::float4{ normalize(p.xyz), 0 };  // p.w is guaranteed to be 0
		}

		updateBoundingSpheres(mBoundingSpheres,
			mFroxelCountX, mFroxelCountY, mFroxelCountZ,
			planesX, planesY, mDistancesZ);

		// note: none of the values below are affected by the projection offset, scale or rotation.
		float const Pz = mProjection[2][2];
		float const Pw = mProjection[3][2];
		if (mProjection[2][3] != 0) {
			// With our inverted DX convention, we have the simple relation:
			// z_view = -near / z_screen
			// ==> i = log2(-z / far) / linearizer + zcount
			// ==> i = -log2(z_screen * (far/near)) * (1/linearizer) + zcount
			// ==> i = log2(z_screen * (far/near)) * (-1/linearizer) + zcount
			mParamsZ[0] = mZLightFar / Pw;
			mParamsZ[1] = 0.0f;
			mParamsZ[2] = -mLinearizer[1];
		}
		else {
			// orthographic projection
			// z_view = (1 - z_screen) * (near - far) - near
			// z_view = z_screen * (far - near) - far
			// our ortho matrix is in inverted-DX convention
			//   Pz =   1 / (far - near)
			//   Pw = far / (far - near)
			mParamsZ[0] = -1.0f / (Pz * mZLightFar);  // -(far-near) / mZLightFar
			mParamsZ[1] = Pw / (Pz * mZLightFar);  //         far / mZLightFar
			mParamsZ[2] = mLinearizer[1];
		}
		uniformsNeedUpdating = true;
	}
	assert_invariant(mZLightNear >= mNear);

	if (UTILS_UNLIKELY(mDirtyFlags)) {
		mFroxelConfigurationInfo = {
			mFroxelCountX, mFroxelCountY, mFroxelCountZ,
			mViewport.width, mViewport.height,
			mFroxelDimension,
			mZLightFar,
			mLinearizer[0],
			mProjection,
			mClipTransform
		};
	}

	mDirtyFlags = 0;
	return uniformsNeedUpdating;
}

UTILS_NOINLINE
void Froxelizer::updateBoundingSpheres(
    filament::math::float4* const UTILS_RESTRICT boundingSpheres,
    size_t froxelCountX, size_t froxelCountY, size_t froxelCountZ,
    filament::math::float4 const* UTILS_RESTRICT planesX,
    filament::math::float4 const* UTILS_RESTRICT planesY,
    float const* UTILS_RESTRICT planesZ) noexcept {

    //FILAMENT_TRACING_CALL(FILAMENT_TRACING_CATEGORY_FILAMENT);

    // TODO: this could potentially be parallel_for'ized

    /*
     * Now compute the bounding sphere of each froxel, which is needed for spotlights
     * We intersect 3 planes of the frustum to find each 8 corners.
     */

    UTILS_ASSUME(froxelCountX > 0);
    UTILS_ASSUME(froxelCountY > 0);

    for (size_t iz = 0, fi = 0, nz = froxelCountZ; iz < nz; ++iz) {
        filament::math::float4 planes[6];
        planes[4] = filament::math::float4{ 0, 0, 1, planesZ[iz + 0] };
        planes[5] = -filament::math::float4{ 0, 0, 1, planesZ[iz + 1] };
        for (size_t iy = 0, ny = froxelCountY; iy < ny; ++iy) {
            planes[2] = planesY[iy];
            planes[3] = -planesY[iy + 1];
            for (size_t ix = 0, nx = froxelCountX; ix < nx; ++ix) {
                planes[0] = planesX[ix];
                planes[1] = -planesX[ix + 1];

                filament::math::float3 const p0 = filament::planeIntersection(planes[0], planes[2], planes[4]);
                filament::math::float3 const p1 = filament::planeIntersection(planes[1], planes[2], planes[4]);
                filament::math::float3 const p2 = filament::planeIntersection(planes[0], planes[3], planes[4]);
                filament::math::float3 const p3 = filament::planeIntersection(planes[1], planes[3], planes[4]);
                filament::math::float3 const p4 = filament::planeIntersection(planes[0], planes[2], planes[5]);
                filament::math::float3 const p5 = filament::planeIntersection(planes[1], planes[2], planes[5]);
                filament::math::float3 const p6 = filament::planeIntersection(planes[0], planes[3], planes[5]);
                filament::math::float3 const p7 = filament::planeIntersection(planes[1], planes[3], planes[5]);

                filament::math::float3 const c = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) * 0.125f;

                float const d0 = length2(p0 - c);
                float const d1 = length2(p1 - c);
                float const d2 = length2(p2 - c);
                float const d3 = length2(p3 - c);
                float const d4 = length2(p4 - c);
                float const d5 = length2(p5 - c);
                float const d6 = length2(p6 - c);
                float const d7 = length2(p7 - c);

                float const r = std::sqrt(std::max({ d0, d1, d2, d3, d4, d5, d6, d7 }));

                assert_invariant(getFroxelIndex(ix, iy, iz, froxelCountX, froxelCountY) == fi);
                boundingSpheres[fi++] = { c, r };
            }
        }
    }
}

// Helper clamp function
inline int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

size_t Froxelizer::findSliceZ(float const viewSpaceZ) const noexcept {
	// The vastly common case is that z<0, so we always do the math for this case
	// and we "undo" it below otherwise. This works because we're using fast::log2 which
	// doesn't care if given a negative number (we'd have to use abs() otherwise).

	// This whole function is now branch-less.

	int s = int(filament::math::fast::log2(-viewSpaceZ / mZLightFar) * mLinearizer[1] + float(mFroxelCountZ));

	// there are cases where z can be negative here, e.g.:
	// - the light is visible, but its center is behind the camera
	// - the camera's near is behind the camera (e.g. with shadowmap cameras)
	// in that case just return the first slice
	s = viewSpaceZ < 0 ? s : 0;

	// clamp between [0, mFroxelCountZ)
	return size_t(clamp(s, 0, mFroxelCountZ - 1));
}

std::pair<size_t, size_t> Froxelizer::clipToIndices(filament::math::float2 const& clip) const noexcept {
	// clip coordinates between [-1, 1], conversion to index between [0, count[
	// (clip + 1) * 0.5 * dimension / froxelsize
	// clip * 0.5 * dimension / froxelsize + 0.5 * dimension / froxelsize
	const size_t xi = size_t(clamp(int(clip.x * mClipToFroxelX + mClipToFroxelX), 0, mFroxelCountX - 1));
	const size_t yi = size_t(clamp(int(clip.y * mClipToFroxelY + mClipToFroxelY), 0, mFroxelCountY - 1));
	return { xi, yi };
}

Froxel Froxelizer::getFroxelAt(size_t const x, size_t const y, size_t const z) const noexcept {
	assert_invariant(x < mFroxelCountX);
	assert_invariant(y < mFroxelCountY);
	assert_invariant(z < mFroxelCountZ);
	Froxel froxel;
	froxel.planes[Froxel::LEFT] = mPlanesX[x];
	froxel.planes[Froxel::BOTTOM] = mPlanesY[y];
	froxel.planes[Froxel::NEAR] = filament::math::float4{ 0, 0, 1, mDistancesZ[z] };
	froxel.planes[Froxel::RIGHT] = -mPlanesX[x + 1];
	froxel.planes[Froxel::TOP] = -mPlanesY[y + 1];
	froxel.planes[Froxel::FAR] = -filament::math::float4{ 0, 0, 1, mDistancesZ[z + 1] };
	return froxel;
}

void Froxelizer::froxelizeAssignRecordsCompress() noexcept {
	utils::Slice<const Froxelizer::FroxelThreadData> froxelThreadData = mFroxelShardedData;

	// Convert froxel data from N groups of M bits to LightRecord::bitset, so we can
	// easily compare adjacent froxels, for compaction. The conversion loops below get
	// inlined and vectorized in release builds.

	// this gets very well vectorized...

	utils::Slice<LightRecord> records(mLightRecords);
	for (size_t j = 0, jc = getFroxelBufferEntryCount(); j < jc; j++) {
		using container_type = LightRecord::bitset::container_type;
		constexpr size_t r = sizeof(container_type) / sizeof(LightGroupType);
		UTILS_UNROLL
			for (size_t i = 0; i < LightRecord::bitset::WORLD_COUNT; i++) {
				container_type b = froxelThreadData[i * r][j];
				UTILS_UNROLL
					for (size_t k = 0; k < r; k++) {
						b |= (container_type(froxelThreadData[i * r + k][j]) << (LIGHT_PER_GROUP * k));
					}
				records[j].lights.getBitsAt(i) = b;
			}
	}

	LightRecord::bitset allLights{};
	for (size_t j = 0, jc = getFroxelBufferEntryCount(); j < jc; j++) {
		allLights |= records[j].lights;
	}

	uint16_t offset = 0;
	FroxelEntry* const UTILS_RESTRICT froxels = mFroxelBufferUser.data();

	const size_t froxelCountX = mFroxelCountX;
	RecordBufferType* const UTILS_RESTRICT froxelRecords = mRecordBufferUser.data();

	// Initialize the first record with all lights in the scene -- this will be used only if
	// we run out of record space.

	// Our light count cannot be larger than 255 because it's stored in a uint8_t. This should
	// be guaranteed by CONFIG_MAX_LIGHT_COUNT
	assert_invariant(allLights.count() <= std::numeric_limits<uint8_t>::max());

	const uint8_t allLightsCount = allLights.count();
	offset += allLightsCount;
	allLights.forEachSetBit([p = froxelRecords](size_t l) mutable {
		// make sure to keep this code branch-less
		const size_t word = l / LIGHT_PER_GROUP;
		const size_t bit = l % LIGHT_PER_GROUP;
		l = (bit * GROUP_COUNT) | (word % GROUP_COUNT);
		*p++ = RecordBufferType(l);
		});

	// how many froxel record entries were reused (for debugging)
	UTILS_UNUSED size_t reused = 0;

	for (size_t i = 0, c = mFroxelCount; i < c;) {
		LightRecord b = records[i];
		if (b.lights.none()) {
			froxels[i++].u32 = 0;
			continue;
		}

		// We have a limitation of 255 spot + 255 point lights per froxel.
		assert_invariant(b.lights.count() <= std::numeric_limits<uint8_t>::max());

		// note: initializer list for union cannot have more than one element
		FroxelEntry entry{ offset, uint8_t(b.lights.count()) };
		const size_t lightCount = entry.count();

		if (UTILS_UNLIKELY(offset + lightCount >= mFroxelRecordBufferEntryCount)) {
			// DLOG(INFO) << "out of space: " << i << ", at " << offset;
			// note: instead of dropping froxels we could look for similar records we've already
			// filed up.
			do {
				froxels[i] = { 0u, allLightsCount };
				if (records[i].lights.none()) {
					froxels[i].u32 = 0;
				}
			} while (++i < c);
			goto out_of_memory;
		}

		// iterate the bitfield
		auto* const beginPoint = froxelRecords + offset;
		b.lights.forEachSetBit([p = beginPoint](size_t l) mutable {
			// make sure to keep this code branch-less
			const size_t word = l / LIGHT_PER_GROUP;
			const size_t bit = l % LIGHT_PER_GROUP;
			l = (bit * GROUP_COUNT) | (word % GROUP_COUNT);
			*p++ = RecordBufferType(l);
			});

		offset += lightCount;

#ifndef NDEBUG
		if (lightCount) { reused--; }
#endif
		do {
#ifndef NDEBUG
			if (lightCount) { reused++; }
#endif
			froxels[i++].u32 = entry.u32;
			if (i >= c) break;

			if (records[i].lights != b.lights && i >= froxelCountX) {
				// if this froxel record doesn't match the previous one on its left,
				// we re-try with the record above it, which saves many froxel records
				// (north of 10% in practice).
				b = records[i - froxelCountX];
				entry.u32 = froxels[i - froxelCountX].u32;
			}
		} while (records[i].lights == b.lights);
	}
out_of_memory:
	// FIXME: on big-endian systems we need to change the endianness of the record buffer
	;
}

static filament::math::float2 project(filament::math::mat4f const& p, filament::math::float3 const& v) noexcept {
	const float vx = v[0];
	const float vy = v[1];
	const float vz = v[2];
	const float x = p[0].x * vx + p[1].x * vy + p[2].x * vz + p[3].x;
	const float y = p[0].y * vx + p[1].y * vy + p[2].y * vz + p[3].y;
	const float w = p[0].w * vx + p[1].w * vy + p[2].w * vz + p[3].w;
	return filament::math::float2{ x, y } *(1.0f / w);
}

void Froxelizer::froxelizePointAndSpotLight(
	FroxelThreadData& froxelThread, size_t bit,
	filament::math::mat4f const& UTILS_RESTRICT projection,
	const LightParams& UTILS_RESTRICT light) const noexcept {

	if (UTILS_UNLIKELY(light.position.z + light.radius < -mZLightFar)) { // z values are negative
		// This light is fully behind LightFar, it doesn't light anything
		// (we could avoid this check if we culled lights using LightFar instead of the
		// culling camera's far plane)
		return;
	}

	// the code below works with radius^2
	const filament::math::float4 s = { light.position, light.radius * light.radius };

#ifdef DEBUG_FROXEL
	const size_t x0 = 0;
	const size_t x1 = mFroxelCountX - 1;
	const size_t y0 = 0;
	const size_t y1 = mFroxelCountY - 1;
	const size_t z0 = 0;
	const size_t z1 = mFroxelCountZ - 1;
#else
	// find a reasonable bounding-box in froxel space for the sphere by projecting
	// its (clipped) bounding-box to clip-space and converting to froxel indices.
	filament::Box const aabb = { light.position, light.radius };
	const float znear = std::min(-mNear, aabb.center.z + aabb.halfExtent.z); // z values are negative
	const float zfar = aabb.center.z - aabb.halfExtent.z;

	// TODO: we need to investigate if doing all this actually saves time
	//       e.g.: we could only do the z-min/max which is much easier to compute.

	const filament::math::float2 pts[8] = {
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{ 1, 1 }, znear }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{ 1,-1 }, znear }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{-1, 1 }, znear }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{-1,-1 }, znear }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{ 1, 1 }, zfar  }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{ 1,-1 }, zfar  }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{-1, 1 }, zfar  }),
		project(projection, { aabb.center.xy + aabb.halfExtent.xy * filament::math::float2{-1,-1 }, zfar  }),
	};

	filament::math::float2 pmin = std::numeric_limits<float>::max();
	filament::math::float2 pmax = 0;
	for (auto pt : pts) {
		pmin = min(pmin, pt);
		pmax = max(pmax, pt);
	}

	//const auto [x0, y0] = clipToIndices(pmin);
	std::pair<size_t, size_t>& pminIndices = clipToIndices(pmin);
	size_t const x0 = pminIndices.first;
	size_t const y0 = pminIndices.second;
	const size_t z0 = findSliceZ(znear);

	//const auto [x1, y1] = clipToIndices(pmax);
	std::pair<size_t, size_t>& pmaxIndices = clipToIndices(pmax);
	size_t const x1 = pmaxIndices.first;
	size_t const y1 = pmaxIndices.second;
	const size_t z1 = findSliceZ(zfar);

	assert_invariant(x0 <= x1);
	assert_invariant(y0 <= y1);
	assert_invariant(z0 <= z1);

#endif

	const size_t zcenter = findSliceZ(s.z);
	filament::math::float4 const* const UTILS_RESTRICT planesX = mPlanesX;
	filament::math::float4 const* const UTILS_RESTRICT planesY = mPlanesY;
	float const* const UTILS_RESTRICT planesZ = mDistancesZ;
	filament::math::float4 const* const UTILS_RESTRICT boundingSpheres = mBoundingSpheres;
	for (size_t iz = z0; iz <= z1; ++iz) {
		filament::math::float4 cz(s);
		// froxel that contain the center of the sphere is special, we don't even need to do the
		// intersection check, it's always true.
		if (UTILS_LIKELY(iz != zcenter)) {
			cz = filament::spherePlaneIntersection(s, (iz < zcenter) ? planesZ[iz + 1] : planesZ[iz]);
		}

		if (cz.w > 0) { // intersection of light with this plane (slice)
			// the sphere (light) intersects this slice's plane, and we now have a new smaller
			// sphere centered there. Now, find x & y slices that contain the sphere's center
			// (note: this changes with the Z slices)
			const filament::math::float2 clip = project(projection, cz.xyz);
			//auto const [xcenter, ycenter] = clipToIndices(clip);
			std::pair<size_t, size_t>& centerIndices = clipToIndices(clip);
			size_t const xcenter = centerIndices.first;
			size_t const ycenter = centerIndices.second;

			for (size_t iy = y0; iy <= y1; ++iy) {
				filament::math::float4 cy(cz);
				// froxel that contain the center of the sphere is special, we don't even need to
				// do the intersection check, it's always true.
				if (UTILS_LIKELY(iy != ycenter)) {
					filament::math::float4 const plane = iy < ycenter ? planesY[iy + 1] : planesY[iy];
					cy = filament::spherePlaneIntersection(cz, plane);
				}

				if (cy.w > 0) {
					// The reduced sphere from the previous stage intersects this horizontal plane,
					// and we now have new smaller sphere centered on these two previous planes
					size_t bx = std::numeric_limits<size_t>::max(); // horizontal begin index
					size_t ex = 0; // horizontal end index

					// find the "begin" index (left side)
					for (size_t ix = x0; ix < x1 + 1; ++ix) {
						// The froxel that contains the center of the sphere is special,
						// we don't even need to do the intersection check, it's always true.
						if (UTILS_LIKELY(ix != xcenter)) {
							filament::math::float4 const plane = ix < xcenter ? planesX[ix + 1] : planesX[ix];
							if (filament::spherePlaneIntersection(cy, plane).w > 0) {
								// The reduced sphere from the previous stage intersects this
								// vertical plane, we record the min/max froxel indices
								bx = std::min(bx, ix);
								ex = std::max(ex, ix);
							}
						}
						else {
							// this is the froxel containing the center of the sphere, it is
							// definitely participating
							bx = std::min(bx, ix);
							ex = std::max(ex, ix);
						}
					}

					if (UTILS_UNLIKELY(bx > ex)) {
						continue;
					}

					// the loops below assume 1-past the end for the right side of the range
					ex++;
					assert_invariant(bx <= mFroxelCountX && ex <= mFroxelCountX);

					size_t fi = getFroxelIndex(bx, iy, iz);
					if (light.invSin != std::numeric_limits<float>::infinity()) {
						// This is a spotlight (common case)
						// this loops gets vectorized (on arm64) w/ clang
						while (bx++ != ex) {
							// see if this froxel intersects the cone
							bool const intersect = filament::sphereConeIntersectionFast(boundingSpheres[fi],
								light.position, light.axis, light.invSin, light.cosSqr);
							froxelThread[fi++] |= LightGroupType(intersect) << bit;
						}
					}
					else {
						// this loops gets vectorized (on arm64) w/ clang
						while (bx++ != ex) {
							froxelThread[fi++] |= LightGroupType(1) << bit;
						}
					}
				}
			}
		}
	}
}

void Froxelizer::froxelizeLights(filament::math::mat4f const& UTILS_RESTRICT viewMatrix,
	const std::vector<Light*>& lights) noexcept {
	// note: this is called asynchronously
	froxelizeLoop(viewMatrix, lights);
	froxelizeAssignRecordsCompress();

#ifndef NDEBUG
	if (lights.size()) {
		// go through every froxel
		auto const& recordBufferUser(mRecordBufferUser);
		auto gpuFroxelEntries(mFroxelBufferUser);
		gpuFroxelEntries.set(gpuFroxelEntries.begin(),
			mFroxelCountX * mFroxelCountY * mFroxelCountZ);
		for (auto const& entry : gpuFroxelEntries) {
			// go through every light for that froxel
			for (size_t i = 0; i < entry.count(); i++) {
				// get the light index
				assert_invariant(entry.offset() + i < mFroxelRecordBufferEntryCount);

				size_t const lightIndex = recordBufferUser[entry.offset() + i];
				assert_invariant(lightIndex <= CONFIG_MAX_LIGHT_INDEX);

				// make sure it corresponds to an existing light
				assert_invariant(lightIndex < lights.size() - Light::DIRECTIONAL_LIGHTS_COUNT);
			}
		}
	}
#endif
}

void Froxelizer::froxelizeLoop(const filament::math::mat4f& UTILS_RESTRICT viewMatrix,
	const std::vector<Light*>& lights) const noexcept {

	utils::Slice<FroxelThreadData> froxelThreadData = mFroxelShardedData;
	memset(froxelThreadData.data(), 0, froxelThreadData.sizeInBytes());

	auto process = [this, &froxelThreadData,
		&viewMatrix, &lights]
		(size_t const count, size_t const offset, size_t const stride) {


		const filament::math::mat4f& projection = mProjection;
		const filament::math::mat3f& vn = viewMatrix.upperLeft();

		// We use minimum cone angle of 0.5 degrees because too small angles cause issues in the
		// sphere/cone intersection test, due to floating-point precision.
		constexpr float maxInvSin = 114.59301f;         // 1 / sin(0.5 degrees)
		constexpr float maxCosSquared = 0.99992385f;    // cos(0.5 degrees)^2

		for (size_t i = offset; i < count; i += stride) {
			const size_t j = i + Light::DIRECTIONAL_LIGHTS_COUNT;
			const Light* li = lights[j];
			LightParams light;
			const Vector3& pos = li->GetTransform()->GetPosition();
			const Vector3& dir = -li->GetTransform()->GetForward();
			light.position = (viewMatrix * filament::math::float4(pos.x, pos.y, pos.z, 1)).xyz;     // to view-space
			light.cosSqr = std::min(maxCosSquared, li->getCosOuterSquared());  // spot only
			light.axis = vn * filament::math::float3(dir.x, dir.y, dir.z);      // spot only
			light.invSin = li->getSinInverse();                                  // spot only
			light.radius = li->GetRange();
			// infinity means "point-light"
			if (light.invSin != std::numeric_limits<float>::infinity()) {
				light.invSin = std::min(maxInvSin, light.invSin);
			}

			const size_t group = i % GROUP_COUNT;
			const size_t bit = i / GROUP_COUNT;
			assert_invariant(bit < LIGHT_PER_GROUP);

			FroxelThreadData& threadData = froxelThreadData[group];
			froxelizePointAndSpotLight(threadData, bit, projection, light);
		}
	};

	// we do 64 lights per job
	utils::JobSystem* js = Engine::Instance()->getJobSystem();

	constexpr bool SINGLE_THREADED = false;
	if (!SINGLE_THREADED) {
		auto* parent = js->createJob();
		for (size_t i = 0; i < GROUP_COUNT; i++) {
			js->run(utils::jobs::createJob(*js, parent, std::cref(process),
				lights.size() - Light::DIRECTIONAL_LIGHTS_COUNT, i, GROUP_COUNT));
		}
		js->runAndWait(parent);
	}
	else {
		js->runAndWait(utils::jobs::createJob(*js, nullptr, std::cref(process),
			lights.size() - Light::DIRECTIONAL_LIGHTS_COUNT, 0, 1)
		);
	}
}

void Froxelizer::commit(filament::backend::DriverApi& driverApi) {
    // Send data to GPU
    if (mFroxelsBuffer) {
        driverApi.loadUniformBuffer(mFroxelsBuffer,
            { reinterpret_cast<const uint8_t*>(mFroxelBufferUser.data()), 
              mFroxelBufferEntryCount * sizeof(FroxelEntry) });
    }

    if (mRecordsBuffer) {
        driverApi.loadUniformBuffer(mRecordsBuffer,
            { reinterpret_cast<const uint8_t*>(mRecordBufferUser.data()), 
              mRecordBufferUser.size() * sizeof(RecordBufferType) });
    }
}
