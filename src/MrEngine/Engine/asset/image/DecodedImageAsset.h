#pragma once

#include "asset/Asset.h"
#include "asset/image/ImageDecodeTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ImageDecode {

class DecodedImageAsset : public AsyncLoad::Asset {
public:
    DecodedImageAsset()
        : width(0), height(0), channels(0), format(DecodedImageFormat::Unknown) {}

    std::string debugName;
    int width;
    int height;
    int channels;
    DecodedImageFormat format;
    std::vector<std::uint8_t> pixels;
};

} // namespace ImageDecode
