#pragma once

#include "asset/AssetLoader.h"

#include <memory>
#include <typeindex>
#include <vector>

namespace AsyncLoad {

class AssetRegistry {
public:
    void RegisterLoader(std::unique_ptr<IAssetLoader> loader);
    IAssetLoader* FindLoader(std::type_index type, const AssetLoadRequest& request) const;

private:
    std::vector<std::unique_ptr<IAssetLoader> > loaders_;
};

} // namespace AsyncLoad
