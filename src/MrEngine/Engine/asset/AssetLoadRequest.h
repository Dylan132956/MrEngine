#pragma once

#include "asset/AssetId.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace AsyncLoad {

enum class AssetSourceKind {
    Path,
    Memory,
    Virtual
};

struct AssetLoadRequest {
    AssetId id;
    AssetSourceKind sourceKind;
    std::string path;
    std::vector<std::uint8_t> bytes;
    std::shared_ptr<const std::vector<std::uint8_t> > sharedBytes;
    std::shared_ptr<const void> userData;
    std::string mimeType;
    std::string debugName;

    AssetLoadRequest()
        : sourceKind(AssetSourceKind::Virtual) {}

    const std::uint8_t* BytesData() const {
        if (sharedBytes && !sharedBytes->empty()) {
            return sharedBytes->data();
        }
        return bytes.empty() ? nullptr : bytes.data();
    }

    std::size_t BytesSize() const {
        return sharedBytes ? sharedBytes->size() : bytes.size();
    }

    bool HasBytes() const {
        return BytesSize() > 0;
    }

    static AssetLoadRequest FromPath(const std::string& path) {
        AssetLoadRequest request;
        request.id = AssetId(path);
        request.sourceKind = AssetSourceKind::Path;
        request.path = path;
        request.debugName = path;
        return request;
    }

    static AssetLoadRequest FromMemory(const std::string& id,
                                       const std::vector<std::uint8_t>& bytes,
                                       const std::string& mimeType) {
        AssetLoadRequest request;
        request.id = AssetId(id);
        request.sourceKind = AssetSourceKind::Memory;
        request.bytes = bytes;
        request.mimeType = mimeType;
        request.debugName = id;
        return request;
    }

    static AssetLoadRequest FromSharedMemory(const std::string& id,
                                             const std::shared_ptr<const std::vector<std::uint8_t> >& bytes,
                                             const std::string& mimeType) {
        AssetLoadRequest request;
        request.id = AssetId(id);
        request.sourceKind = AssetSourceKind::Memory;
        request.sharedBytes = bytes;
        request.mimeType = mimeType;
        request.debugName = id;
        return request;
    }

    static AssetLoadRequest FromVirtual(const std::string& id) {
        AssetLoadRequest request;
        request.id = AssetId(id);
        request.sourceKind = AssetSourceKind::Virtual;
        request.debugName = id;
        return request;
    }
};

} // namespace AsyncLoad
