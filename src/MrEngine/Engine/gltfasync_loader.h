#pragma once

#include "asset/AssetLoader.h"
#include "sceneasync.h"

namespace moonriver {

class GLTFAsyncCpuLoader : public AsyncLoad::TypedAssetLoader<SceneAsyncCpuAsset> {
public:
    bool CanLoad(const AsyncLoad::AssetLoadRequest& request) const override;
    AsyncLoad::LoadResult Load(const AsyncLoad::AssetLoadRequest& request,
                               AsyncLoad::IFileSystem& fileSystem,
                               const AsyncLoad::LoadContext& context) override;
};

} // namespace moonriver
