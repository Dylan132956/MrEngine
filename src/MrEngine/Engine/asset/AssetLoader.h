#pragma once

#include "asset/Asset.h"
#include "asset/AssetLoadRequest.h"
#include "asset/FileSystem.h"
#include "asset/Result.h"

#include <typeindex>
#include <typeinfo>

namespace AsyncLoad {

class IAssetLoader {
public:
    virtual ~IAssetLoader() {}
    virtual std::type_index GetAssetType() const = 0;
    virtual bool CanLoad(const AssetLoadRequest& request) const = 0;
    virtual LoadResult Load(const AssetLoadRequest& request, IFileSystem& fileSystem, const LoadContext& context) = 0;
};

template <typename T>
class TypedAssetLoader : public IAssetLoader {
public:
    std::type_index GetAssetType() const override {
        return std::type_index(typeid(T));
    }
};

} // namespace AsyncLoad
