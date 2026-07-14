#pragma once

#include "asset/AssetManager.h"
#include "asset/image/DecodedImageAsset.h"
#include "asset/image/ImageSource.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ImagePipeline {

struct ImageDecodeBatchDesc {
    std::uint32_t maxSubmitCountPerFrame;
    std::uint64_t maxSubmitBytesPerFrame;
    std::uint32_t maxPollCountPerFrame;

    ImageDecodeBatchDesc()
        : maxSubmitCountPerFrame(4),
          maxSubmitBytesPerFrame(4 * 1024 * 1024),
          maxPollCountPerFrame(8) {}
};

class ImageDecodeBatch {
public:
    ImageDecodeBatch();

    void Reset(const std::vector<ImageSource>& sources, const ImageDecodeBatchDesc& desc = ImageDecodeBatchDesc());
    std::uint32_t Submit(AsyncLoad::AssetManager& assetManager);
    void Poll();
    void Cancel();

    bool IsSubmitComplete() const;
    bool IsDone() const;
    bool HasFailures() const;
    const std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> >& GetDecodedImages() const;

private:
    typedef AsyncLoad::AssetHandle<ImageDecode::DecodedImageAsset> DecodedImageHandle;

    struct PendingImageDecode {
        DecodedImageHandle handle;
        std::size_t imageIndex;
    };

    ImageDecodeBatchDesc desc_;
    std::vector<ImageSource> sources_;
    std::vector<PendingImageDecode> pending_;
    std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> > decodedImages_;
    std::size_t nextSubmitIndex_;
    bool hasFailures_;
};

} // namespace ImagePipeline
