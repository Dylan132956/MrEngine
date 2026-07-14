#include "asset/AssetRegistry.h"

namespace AsyncLoad {

void AssetRegistry::RegisterLoader(std::unique_ptr<IAssetLoader> loader) {
    if (loader) {
        loaders_.push_back(std::move(loader));
    }
}

IAssetLoader* AssetRegistry::FindLoader(std::type_index type, const AssetLoadRequest& request) const {
    for (std::size_t i = 0; i < loaders_.size(); ++i) {
        IAssetLoader* loader = loaders_[i].get();
        if (loader != nullptr && loader->GetAssetType() == type && loader->CanLoad(request)) {
            return loader;
        }
    }
    return nullptr;
}

} // namespace AsyncLoad
