#pragma once

#include "asset/Asset.h"
#include "asset/AssetId.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace AsyncLoad {

enum class AssetLoadState {
    None,
    Queued,
    Loading,
    Completed,
    Failed,
    Cancelled
};

struct FileReadResult {
    bool success;
    std::vector<std::uint8_t> data;
    std::string error;

    FileReadResult() : success(false) {}
};

struct LoadContext {
    std::atomic<bool>* cancelled;
    class AssetManager* manager;

    LoadContext() : cancelled(nullptr), manager(nullptr) {}

    bool IsCancelled() const {
        return cancelled != nullptr && cancelled->load();
    }
};

struct LoadResult {
    bool success;
    std::shared_ptr<Asset> asset;
    std::vector<AssetId> dependencies;
    std::string error;

    LoadResult() : success(false) {}

    static LoadResult Succeeded(const std::shared_ptr<Asset>& loadedAsset) {
        LoadResult result;
        result.success = true;
        result.asset = loadedAsset;
        return result;
    }

    static LoadResult Failed(const std::string& message) {
        LoadResult result;
        result.success = false;
        result.error = message;
        return result;
    }
};

} // namespace AsyncLoad
