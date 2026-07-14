#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ImagePipeline {

struct ImageSource {
    std::string id;
    std::string name;
    std::string mimeType;
    std::vector<std::uint8_t> encodedBytes;
    std::shared_ptr<const std::vector<std::uint8_t> > sharedBytes;

    std::size_t BytesSize() const {
        return sharedBytes ? sharedBytes->size() : encodedBytes.size();
    }

    bool HasBytes() const {
        return BytesSize() > 0;
    }
};

} // namespace ImagePipeline
