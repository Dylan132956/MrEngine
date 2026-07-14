#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace RenderUpload {

enum class UploadTaskType {
    Texture2D,
    VertexBuffer,
    IndexBuffer,
    MaterialBinding,
    Custom
};

enum class UploadPriority {
    Low = 0,
    Normal = 100,
    High = 200,
    Critical = 300
};

struct UploadHandle {
    std::uint64_t id;

    UploadHandle() : id(0) {}
    explicit UploadHandle(std::uint64_t value) : id(value) {}

    bool IsValid() const { return id != 0; }
    bool operator==(const UploadHandle& rhs) const { return id == rhs.id; }
    bool operator!=(const UploadHandle& rhs) const { return !(*this == rhs); }
};

struct UploadRequest {
    UploadHandle handle;
    std::uint64_t ownerId;
    UploadTaskType type;
    UploadPriority priority;
    std::uint64_t byteSize;
    std::string debugName;
    std::shared_ptr<const std::vector<std::uint8_t> > payload;
    int width;
    int height;
    int format;
    int usage;

    UploadRequest()
        : ownerId(0),
          type(UploadTaskType::Custom),
          priority(UploadPriority::Normal),
          byteSize(0),
          width(0),
          height(0),
          format(0),
          usage(0) {}

    const std::uint8_t* PayloadData() const {
        return payload && !payload->empty() ? payload->data() : nullptr;
    }

    std::size_t PayloadSize() const {
        return payload ? payload->size() : 0;
    }

    bool HasPayload() const {
        return PayloadSize() > 0;
    }
};

struct UploadBudget {
    std::uint32_t maxTaskCount;
    std::uint64_t maxBytes;
    std::uint32_t maxTextureCount;
    std::uint32_t maxBufferCount;

    UploadBudget()
        : maxTaskCount(4),
          maxBytes(8 * 1024 * 1024),
          maxTextureCount(2),
          maxBufferCount(4) {}
};

} // namespace RenderUpload
