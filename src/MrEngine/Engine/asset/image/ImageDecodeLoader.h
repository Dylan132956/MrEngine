#pragma once

#include "asset/AssetLoader.h"
#include "asset/image/DecodedImageAsset.h"

namespace ImageDecode {

class ImageDecodeLoader : public AsyncLoad::TypedAssetLoader<DecodedImageAsset> {
public:
    bool CanLoad(const AsyncLoad::AssetLoadRequest& request) const override;
    AsyncLoad::LoadResult Load(const AsyncLoad::AssetLoadRequest& request,
                               AsyncLoad::IFileSystem& fileSystem,
                               const AsyncLoad::LoadContext& context) override;
};

} // namespace ImageDecode
