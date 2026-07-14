#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ImageDecode {

enum class DecodedImageFormat {
    Unknown,
    RGBA8
};

struct ImageDecodeRequest {
    std::uint64_t id;
    std::string debugName;
    std::string mimeType;
    std::vector<std::uint8_t> encodedBytes;
    int priority;

    ImageDecodeRequest() : id(0), priority(0) {}
};

struct DecodedImage {
    std::uint64_t id;
    std::string debugName;
    int width;
    int height;
    int channels;
    DecodedImageFormat format;
    std::vector<std::uint8_t> pixels;
    std::string error;

    DecodedImage()
        : id(0), width(0), height(0), channels(0), format(DecodedImageFormat::Unknown) {}

    bool Succeeded() const {
        return error.empty() && !pixels.empty();
    }
};

} // namespace ImageDecode
