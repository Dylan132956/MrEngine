#include "asset/image/TextureUploadBatch.h"

#include "graphics/Texture.h"
#include "memory/ByteBuffer.h"

namespace ImagePipeline {

std::uint64_t TextureUploadBatch::nextBatchId_ = 1;

TextureUploadBatch::TextureUploadBatch()
    : batchId_(nextBatchId_++) {}

void TextureUploadBatch::Reset(const std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> >& images,
                               RenderUpload::FrameUploadScheduler& scheduler,
                               const std::shared_ptr<moonriver::Texture>& fallbackTexture) {
    uploadHandles_.clear();
    pendingUploads_.clear();
    textures_.assign(images.size(), fallbackTexture);

    for (std::size_t i = 0; i < images.size(); ++i) {
        const std::shared_ptr<ImageDecode::DecodedImageAsset>& decoded = images[i];
        if (!decoded || decoded->pixels.empty()) {
            textures_[i] = fallbackTexture;
            continue;
        }

        RenderUpload::UploadRequest request;
        request.ownerId = batchId_;
        request.type = RenderUpload::UploadTaskType::Texture2D;
        request.priority = RenderUpload::UploadPriority::Normal;
        request.debugName = decoded->debugName;
        request.width = decoded->width;
        request.height = decoded->height;
        request.format = static_cast<int>(moonriver::TextureFormat::R8G8B8A8);
        request.payload = std::shared_ptr<const std::vector<std::uint8_t> >(decoded, &decoded->pixels);
        request.byteSize = static_cast<std::uint64_t>(request.PayloadSize());

        RenderUpload::UploadHandle uploadHandle = scheduler.Enqueue(request);
        uploadHandles_.push_back(uploadHandle);

        PendingTextureUpload pending;
        pending.handle = uploadHandle;
        pending.textureIndex = i;
        pendingUploads_.push_back(pending);
    }
}

std::uint32_t TextureUploadBatch::SubmitFrame(RenderUpload::FrameUploadScheduler& scheduler,
                                              const RenderUpload::UploadBudget& budget) {
    return scheduler.SubmitFrameForOwner(batchId_, budget, [this, &scheduler](const RenderUpload::UploadRequest& request) {
        std::size_t foundIndex = pendingUploads_.size();
        for (std::size_t i = 0; i < pendingUploads_.size(); ++i) {
            if (pendingUploads_[i].handle == request.handle) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex >= pendingUploads_.size()) {
            return;
        }

        moonriver::ByteBuffer buffer(const_cast<moonriver::byte*>(request.PayloadData()), static_cast<int>(request.PayloadSize()));
        textures_[pendingUploads_[foundIndex].textureIndex] = moonriver::Texture::CreateTexture2DFromMemory(
            buffer,
            request.width,
            request.height,
            static_cast<moonriver::TextureFormat>(request.format),
            moonriver::FilterMode::Linear,
            moonriver::SamplerAddressMode::Repeat,
            true);
        scheduler.MarkCompleted(request.handle);
    });
}

bool TextureUploadBatch::IsDone(const RenderUpload::FrameUploadScheduler& scheduler) const {
    for (std::size_t i = 0; i < uploadHandles_.size(); ++i) {
        if (!scheduler.IsCompleted(uploadHandles_[i])) {
            return false;
        }
    }
    return true;
}

const std::vector<std::shared_ptr<moonriver::Texture> >& TextureUploadBatch::GetTextures() const {
    return textures_;
}

void TextureUploadBatch::Clear() {
    uploadHandles_.clear();
    pendingUploads_.clear();
    textures_.clear();
}

} // namespace ImagePipeline
