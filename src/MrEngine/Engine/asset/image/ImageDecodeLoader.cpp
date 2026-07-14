#include "asset/image/ImageDecodeLoader.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#include <limits>
#include <memory>
#include "graphics/Image.h"
#include "memory/ByteBuffer.h"

using namespace moonriver;

namespace ImageDecode {

bool ImageDecodeLoader::CanLoad(const AsyncLoad::AssetLoadRequest& request) const {
    return request.sourceKind == AsyncLoad::AssetSourceKind::Memory && request.HasBytes();
}

AsyncLoad::LoadResult ImageDecodeLoader::Load(const AsyncLoad::AssetLoadRequest& request,
                                              AsyncLoad::IFileSystem&,
                                              const AsyncLoad::LoadContext& context) {
    if (context.IsCancelled()) {
        return AsyncLoad::LoadResult::Failed("Image decode cancelled");
    }

    if (!request.HasBytes()) {
        return AsyncLoad::LoadResult::Failed("Image decode input is empty");
    }

    if (request.BytesSize() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return AsyncLoad::LoadResult::Failed("Image decode input is too large");
    }

    int width = 0;
    int height = 0;
    int sourceChannels = 0;

    ByteBuffer buffer(const_cast<byte*>(request.BytesData()), request.BytesSize());

	auto image = Image::LoadFromMemory(buffer);
	if (!image) {
		return AsyncLoad::LoadResult::Failed("Image decode failed");
	}
    byte* pixels = image->data.Bytes();
    width = image->width;
    height = image->height;
    //stbi_uc* pixels = stbi_load_from_memory(request.BytesData(),
    //                                        static_cast<int>(request.BytesSize()),
    //                                        &width,
    //                                        &height,
    //                                        &sourceChannels,
    //                                        4);
    //if (!pixels) {
    //    const char* reason = stbi_failure_reason();
    //    return AsyncLoad::LoadResult::Failed(reason ? reason : "Image decode failed");
    //}

    //if (context.IsCancelled()) {
    //    stbi_image_free(pixels);
    //    return AsyncLoad::LoadResult::Failed("Image decode cancelled");
    //}

    const std::size_t byteSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    std::shared_ptr<DecodedImageAsset> asset(new DecodedImageAsset());
    asset->debugName = request.debugName.empty() ? request.id.ToString() : request.debugName;
    asset->width = width;
    asset->height = height;
    asset->channels = 4;
    asset->format = DecodedImageFormat::RGBA8;
    asset->pixels.assign(pixels, pixels + byteSize);
    //stbi_image_free(pixels);

    return AsyncLoad::LoadResult::Succeeded(asset);
}

} // namespace ImageDecode
