#pragma once

#include "asset/image/DecodedImageAsset.h"
#include "renderupload/FrameUploadScheduler.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace moonriver {
class Texture;
}

namespace ImagePipeline {

class TextureUploadBatch {
public:
    TextureUploadBatch();

    void Reset(const std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> >& images,
               RenderUpload::FrameUploadScheduler& scheduler,
               const std::shared_ptr<moonriver::Texture>& fallbackTexture);

    std::uint32_t SubmitFrame(RenderUpload::FrameUploadScheduler& scheduler,
                              const RenderUpload::UploadBudget& budget);

    bool IsDone(const RenderUpload::FrameUploadScheduler& scheduler) const;
    const std::vector<std::shared_ptr<moonriver::Texture> >& GetTextures() const;
    void Clear();

private:
    struct PendingTextureUpload {
        RenderUpload::UploadHandle handle;
        std::size_t textureIndex;
    };

    std::uint64_t batchId_;
    static std::uint64_t nextBatchId_;
    std::vector<RenderUpload::UploadHandle> uploadHandles_;
    std::vector<PendingTextureUpload> pendingUploads_;
    std::vector<std::shared_ptr<moonriver::Texture> > textures_;
};

} // namespace ImagePipeline
