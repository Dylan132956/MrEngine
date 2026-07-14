#include "asset/AssetManager.h"

namespace AsyncLoad {

AssetManager::AssetManager(const AssetManagerDesc& desc)
    : shuttingDown_(false),
      fileSystem_(desc.fileSystem ? desc.fileSystem : std::shared_ptr<IFileSystem>(new LocalFileSystem())),
      threadPool_(desc.workerThreadCount) {}

AssetManager::~AssetManager() {
    Shutdown();
}

void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader> loader) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!shuttingDown_) {
        registry_.RegisterLoader(std::move(loader));
    }
}

std::shared_ptr<AssetRequest> AssetManager::LoadAsyncInternal(const AssetLoadRequest& loadRequest, std::type_index type) {
    AssetKey key(loadRequest.id, type);

    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<AssetKey, std::weak_ptr<Asset>, AssetKeyHash>::iterator loadedIt = loadedAssets_.find(key);
        if (loadedIt != loadedAssets_.end()) {
            std::shared_ptr<Asset> cached = loadedIt->second.lock();
            if (cached) {
                std::shared_ptr<AssetRequest> completed(new AssetRequest(loadRequest, type));
                completed->state.store(AssetLoadState::Completed);
                completed->asset = cached;
                return completed;
            }
            loadedAssets_.erase(loadedIt);
        }

        std::unordered_map<AssetKey, std::shared_ptr<AssetRequest>, AssetKeyHash>::iterator requestIt = requests_.find(key);
        if (requestIt != requests_.end()) {
            return requestIt->second;
        }

        std::shared_ptr<AssetRequest> request(new AssetRequest(loadRequest, type));
        if (shuttingDown_) {
            request->state.store(AssetLoadState::Failed);
            request->error = "AssetManager is shutting down";
            return request;
        }

        request->state.store(AssetLoadState::Queued);
        requests_[key] = request;

        bool enqueued = threadPool_.Enqueue([this, request]() { ExecuteLoad(request); });
        if (!enqueued) {
            requests_.erase(key);
            request->state.store(AssetLoadState::Failed);
            request->error = "Failed to enqueue asset load request";
            request->cv.notify_all();
        }

        return request;
    }
}

void AssetManager::AddCallback(const std::shared_ptr<AssetRequest>& request, const std::function<void()>& callback) {
    if (!request || !callback) {
        return;
    }

    bool runSoon = false;
    {
        std::lock_guard<std::mutex> lock(request->mutex);
        AssetLoadState state = request->state.load();
        if (state == AssetLoadState::Completed || state == AssetLoadState::Failed || state == AssetLoadState::Cancelled) {
            runSoon = true;
        } else {
            request->callbacks.push_back(callback);
        }
    }

    if (runSoon) {
        std::lock_guard<std::mutex> managerLock(mutex_);
        completedCallbacks_.push(callback);
    }
}

void AssetManager::ExecuteLoad(const std::shared_ptr<AssetRequest>& request) {
    if (!request) {
        return;
    }

    if (request->cancelled.load()) {
        CompleteRequest(request, AssetLoadState::Cancelled, std::shared_ptr<Asset>(), std::string());
        return;
    }

    request->state.store(AssetLoadState::Loading);

    IAssetLoader* loader = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loader = registry_.FindLoader(request->type, request->loadRequest);
    }

    if (loader == nullptr) {
        CompleteRequest(request, AssetLoadState::Failed, std::shared_ptr<Asset>(), "No loader registered for asset: " + request->id.ToString());
        return;
    }

    LoadContext context;
    context.cancelled = &request->cancelled;
    context.manager = this;
    LoadResult result = loader->Load(request->loadRequest, *fileSystem_, context);

    if (request->cancelled.load()) {
        CompleteRequest(request, AssetLoadState::Cancelled, std::shared_ptr<Asset>(), std::string());
        return;
    }

    if (!result.success) {
        CompleteRequest(request, AssetLoadState::Failed, std::shared_ptr<Asset>(), result.error.empty() ? "Asset load failed" : result.error);
        return;
    }

    if (!result.asset) {
        CompleteRequest(request, AssetLoadState::Failed, std::shared_ptr<Asset>(), "Loader returned null asset: " + request->id.ToString());
        return;
    }

    CompleteRequest(request, AssetLoadState::Completed, result.asset, std::string());
}

void AssetManager::CompleteRequest(const std::shared_ptr<AssetRequest>& request, AssetLoadState state, const std::shared_ptr<Asset>& asset, const std::string& error) {
    {
        std::lock_guard<std::mutex> requestLock(request->mutex);
        request->asset = asset;
        request->error = error;
        request->state.store(state);
    }

    if (state == AssetLoadState::Completed && asset) {
        std::lock_guard<std::mutex> lock(mutex_);
        loadedAssets_[AssetKey(request->id, request->type)] = asset;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.erase(AssetKey(request->id, request->type));
    }

    request->cv.notify_all();
    QueueRequestCallbacks(request);
}

void AssetManager::QueueRequestCallbacks(const std::shared_ptr<AssetRequest>& request) {
    std::vector<std::function<void()> > callbacks;
    {
        std::lock_guard<std::mutex> lock(request->mutex);
        callbacks.swap(request->callbacks);
    }

    if (callbacks.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (std::size_t i = 0; i < callbacks.size(); ++i) {
        completedCallbacks_.push(callbacks[i]);
    }
}

void AssetManager::DispatchCallbacks() {
    std::queue<std::function<void()> > callbacks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks.swap(completedCallbacks_);
    }

    while (!callbacks.empty()) {
        std::function<void()> callback = callbacks.front();
        callbacks.pop();
        if (callback) {
            callback();
        }
    }
}

void AssetManager::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shuttingDown_) {
            return;
        }
        shuttingDown_ = true;
    }

    threadPool_.Stop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.clear();
        while (!completedCallbacks_.empty()) {
            completedCallbacks_.pop();
        }
    }
}

} // namespace AsyncLoad
