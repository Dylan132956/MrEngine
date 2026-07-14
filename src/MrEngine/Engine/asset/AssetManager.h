#pragma once

#include "asset/AssetHandle.h"
#include "asset/AssetRegistry.h"
#include "asset/FileSystem.h"
#include "asset/ThreadPool.h"

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace AsyncLoad {

struct AssetManagerDesc {
    std::size_t workerThreadCount;
    std::shared_ptr<IFileSystem> fileSystem;

    AssetManagerDesc() : workerThreadCount(0) {}
};

struct AssetKey {
    AssetId id;
    std::type_index type;

    AssetKey(const AssetId& assetId, std::type_index assetType)
        : id(assetId), type(assetType) {}

    bool operator==(const AssetKey& rhs) const {
        return id == rhs.id && type == rhs.type;
    }
};

struct AssetKeyHash {
    std::size_t operator()(const AssetKey& key) const {
        std::size_t pathHash = AssetIdHash()(key.id);
        std::size_t typeHash = key.type.hash_code();
        return pathHash ^ (typeHash + 0x9e3779b9 + (pathHash << 6) + (pathHash >> 2));
    }
};

class AssetManager {
public:
    explicit AssetManager(const AssetManagerDesc& desc = AssetManagerDesc());
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    template <typename T>
    AssetHandle<T> LoadAsync(const AssetLoadRequest& loadRequest, std::function<void(AssetHandle<T>)> callback = std::function<void(AssetHandle<T>)>()) {
        std::shared_ptr<AssetRequest> request = LoadAsyncInternal(loadRequest, std::type_index(typeid(T)));
        AssetHandle<T> handle(request);

        if (callback) {
            AddCallback(request, [handle, callback]() { callback(handle); });
        }

        return handle;
    }

    template <typename T>
    AssetHandle<T> LoadAsync(const std::string& path, std::function<void(AssetHandle<T>)> callback = std::function<void(AssetHandle<T>)>()) {
        return LoadAsync<T>(AssetLoadRequest::FromPath(path), callback);
    }

    void RegisterLoader(std::unique_ptr<IAssetLoader> loader);
    void DispatchCallbacks();
    void Shutdown();

private:
    std::shared_ptr<AssetRequest> LoadAsyncInternal(const AssetLoadRequest& loadRequest, std::type_index type);
    void AddCallback(const std::shared_ptr<AssetRequest>& request, const std::function<void()>& callback);
    void ExecuteLoad(const std::shared_ptr<AssetRequest>& request);
    void CompleteRequest(const std::shared_ptr<AssetRequest>& request, AssetLoadState state, const std::shared_ptr<Asset>& asset, const std::string& error);
    void QueueRequestCallbacks(const std::shared_ptr<AssetRequest>& request);

    mutable std::mutex mutex_;
    bool shuttingDown_;
    AssetRegistry registry_;
    std::shared_ptr<IFileSystem> fileSystem_;
    ThreadPool threadPool_;
    std::unordered_map<AssetKey, std::weak_ptr<Asset>, AssetKeyHash> loadedAssets_;
    std::unordered_map<AssetKey, std::shared_ptr<AssetRequest>, AssetKeyHash> requests_;
    std::queue<std::function<void()> > completedCallbacks_;
};

} // namespace AsyncLoad
