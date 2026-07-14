#pragma once

#include "asset/Asset.h"
#include "asset/AssetLoadRequest.h"
#include "asset/Result.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <vector>

namespace AsyncLoad {

struct AssetRequest {
    AssetRequest(const AssetLoadRequest& request, std::type_index assetType)
        : loadRequest(request), id(request.id), type(assetType), state(AssetLoadState::None), cancelled(false) {}

    AssetLoadRequest loadRequest;
    AssetId id;
    std::type_index type;
    std::atomic<AssetLoadState> state;
    std::atomic<bool> cancelled;
    std::shared_ptr<Asset> asset;
    std::string error;
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::function<void()> > callbacks;
};

template <typename T>
class AssetHandle {
public:
    AssetHandle() {}
    explicit AssetHandle(const std::shared_ptr<AssetRequest>& request) : request_(request) {}

    bool IsValid() const { return request_ != nullptr; }

    bool IsDone() const {
        if (!request_) {
            return true;
        }
        AssetLoadState state = request_->state.load();
        return state == AssetLoadState::Completed ||
               state == AssetLoadState::Failed ||
               state == AssetLoadState::Cancelled;
    }

    bool IsSucceeded() const {
        return request_ && request_->state.load() == AssetLoadState::Completed;
    }

    bool IsFailed() const {
        return request_ && request_->state.load() == AssetLoadState::Failed;
    }

    bool IsCancelled() const {
        return request_ && request_->state.load() == AssetLoadState::Cancelled;
    }

    AssetLoadState GetState() const {
        return request_ ? request_->state.load() : AssetLoadState::None;
    }

    void Cancel() {
        if (request_) {
            request_->cancelled.store(true);
        }
    }

    void Wait() const {
        if (!request_) {
            return;
        }

        std::unique_lock<std::mutex> lock(request_->mutex);
        request_->cv.wait(lock, [this]() { return IsDone(); });
    }

    std::shared_ptr<T> Get() const {
        if (!request_ || request_->state.load() != AssetLoadState::Completed) {
            return std::shared_ptr<T>();
        }

        std::lock_guard<std::mutex> lock(request_->mutex);
        return std::dynamic_pointer_cast<T>(request_->asset);
    }

    std::shared_ptr<T> WaitAndGet() const {
        Wait();
        return Get();
    }

    std::string GetError() const {
        if (!request_) {
            return std::string();
        }

        std::lock_guard<std::mutex> lock(request_->mutex);
        return request_->error;
    }

    std::shared_ptr<AssetRequest> GetRequest() const { return request_; }

private:
    std::shared_ptr<AssetRequest> request_;
};

} // namespace AsyncLoad
