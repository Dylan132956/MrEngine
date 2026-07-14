#include "asset/image/ImageSequence.h"

#include "graphics/Texture.h"

#include <cmath>
#include <cstdio>

namespace ImagePipeline {

bool ImageSequenceCpuLoader::CanLoad(const AsyncLoad::AssetLoadRequest& request) const {
    return request.sourceKind == AsyncLoad::AssetSourceKind::Virtual && request.userData != nullptr;
}

AsyncLoad::LoadResult ImageSequenceCpuLoader::Load(const AsyncLoad::AssetLoadRequest& request,
                                                   AsyncLoad::IFileSystem& fileSystem,
                                                   const AsyncLoad::LoadContext& context) {
    std::shared_ptr<const ImageSequenceDesc> desc = std::static_pointer_cast<const ImageSequenceDesc>(request.userData);
    if (!desc) {
        return AsyncLoad::LoadResult::Failed("Image sequence desc is missing");
    }

    if (desc->framePaths.empty()) {
        return AsyncLoad::LoadResult::Failed("Image sequence frame paths are empty");
    }

    std::shared_ptr<ImageSequenceCpuAsset> asset(new ImageSequenceCpuAsset());
    asset->desc = *desc;
    asset->sources.reserve(desc->framePaths.size());

    for (std::size_t i = 0; i < desc->framePaths.size(); ++i) {
        if (context.IsCancelled()) {
            return AsyncLoad::LoadResult::Failed("Image sequence source load cancelled");
        }

        const std::string& path = desc->framePaths[i];
        AsyncLoad::FileReadResult readResult = fileSystem.ReadAllBytes(path);
        if (!readResult.success) {
            return AsyncLoad::LoadResult::Failed("Image sequence frame read failed: " + path + " " + readResult.error);
        }

        ImageSource source;
        source.id = path;
        source.name = desc->name.empty() ? path : desc->name + "#frame/" + std::to_string(i);
        source.encodedBytes.swap(readResult.data);
        asset->sources.push_back(source);
    }

    return AsyncLoad::LoadResult::Succeeded(asset);
}

ImageSequenceAsset::ImageSequenceAsset()
    : fps(30.0f), loop(true) {}

std::size_t ImageSequenceAsset::GetFrameCount() const {
    return frames.size();
}

std::shared_ptr<moonriver::Texture> ImageSequenceAsset::GetFrame(std::size_t index) const {
    if (index >= frames.size()) {
        return std::shared_ptr<moonriver::Texture>();
    }
    return frames[index];
}

std::shared_ptr<moonriver::Texture> ImageSequenceAsset::Sample(float timeSeconds) const {
    if (frames.empty()) {
        return std::shared_ptr<moonriver::Texture>();
    }
    if (fps <= 0.0f || timeSeconds <= 0.0f) {
        return frames[0];
    }

    const std::size_t frameCount = frames.size();
    std::size_t frameIndex = static_cast<std::size_t>(std::floor(timeSeconds * fps));
    if (loop) {
        frameIndex = frameIndex % frameCount;
    } else if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    }
    return frames[frameIndex];
}

ImageSequenceLoadHandle::ImageSequenceLoadHandle() {}

ImageSequenceLoadHandle::ImageSequenceLoadHandle(const std::shared_ptr<ImageSequenceLoadInstance>& instance)
    : instance_(instance) {}

bool ImageSequenceLoadHandle::IsDone() const {
    return !instance_ || instance_->IsDone();
}

bool ImageSequenceLoadHandle::IsReady() const {
    return instance_ && instance_->IsReady();
}

bool ImageSequenceLoadHandle::IsFailed() const {
    return instance_ && instance_->IsFailed();
}

ImageSequenceLoadState ImageSequenceLoadHandle::GetState() const {
    return instance_ ? instance_->GetState() : ImageSequenceLoadState::Failed;
}

std::shared_ptr<ImageSequenceAsset> ImageSequenceLoadHandle::GetAsset() const {
    return instance_ ? instance_->GetAsset() : std::shared_ptr<ImageSequenceAsset>();
}

std::string ImageSequenceLoadHandle::GetError() const {
    return instance_ ? instance_->GetError() : std::string();
}

ImageSequenceLoadInstance::ImageSequenceLoadInstance(AsyncLoad::AssetManager& assetManager,
                                                     RenderUpload::FrameUploadScheduler& uploadScheduler,
                                                     const ImageSequenceDesc& desc)
    : assetManager_(assetManager),
      uploadScheduler_(uploadScheduler),
      desc_(desc),
      state_(ImageSequenceLoadState::LoadingSources),
      cancelled_(false) {}

void ImageSequenceLoadInstance::Start() {
    if (desc_.framePaths.empty()) {
        Fail("Image sequence frame paths are empty");
        return;
    }

    std::shared_ptr<ImageSequenceDesc> desc(new ImageSequenceDesc(desc_));
    const std::string requestName = desc->name.empty() ? std::string("image_sequence") : desc->name;
    AsyncLoad::AssetLoadRequest request = AsyncLoad::AssetLoadRequest::FromVirtual("image_sequence:" + requestName);
    request.debugName = requestName;
    request.userData = desc;
    cpuHandle_ = assetManager_.LoadAsync<ImageSequenceCpuAsset>(request);
    state_ = ImageSequenceLoadState::LoadingSources;
}

void ImageSequenceLoadInstance::Tick() {
    if (cancelled_) {
        state_ = ImageSequenceLoadState::Cancelled;
        return;
    }

    if (state_ == ImageSequenceLoadState::LoadingSources) {
        if (!cpuHandle_.IsDone()) {
            return;
        }

        if (!cpuHandle_.IsSucceeded()) {
            Fail(cpuHandle_.GetError());
            return;
        }

        cpuAsset_ = cpuHandle_.Get();
        if (!cpuAsset_) {
            Fail("Image sequence CPU asset is null");
            return;
        }

        ImageDecodeBatchDesc decodeDesc;
        imageDecodeBatch_.Reset(cpuAsset_->sources, decodeDesc);
        state_ = imageDecodeBatch_.IsSubmitComplete()
            ? ImageSequenceLoadState::UploadingTextures
            : ImageSequenceLoadState::SubmittingImageDecodes;
        if (state_ == ImageSequenceLoadState::UploadingTextures) {
            BeginTextureUploads();
        }
        return;
    }

    if (state_ == ImageSequenceLoadState::SubmittingImageDecodes) {
        imageDecodeBatch_.Submit(assetManager_);
        imageDecodeBatch_.Poll();
        if (imageDecodeBatch_.IsSubmitComplete()) {
            state_ = imageDecodeBatch_.IsDone() ? ImageSequenceLoadState::UploadingTextures : ImageSequenceLoadState::DecodingImages;
            if (state_ == ImageSequenceLoadState::UploadingTextures) {
                BeginTextureUploads();
            }
        }
        return;
    }

    if (state_ == ImageSequenceLoadState::DecodingImages) {
        imageDecodeBatch_.Poll();
        if (imageDecodeBatch_.IsDone()) {
            state_ = ImageSequenceLoadState::UploadingTextures;
            BeginTextureUploads();
        }
        return;
    }

    if (state_ == ImageSequenceLoadState::UploadingTextures) {
        if (AreTextureUploadsCompleted()) {
            CompleteAsset();
        }
    }
}

std::uint32_t ImageSequenceLoadInstance::SubmitTextureUploads(const RenderUpload::UploadBudget& budget) {
    return textureUploadBatch_.SubmitFrame(uploadScheduler_, budget);
}

void ImageSequenceLoadInstance::Cancel() {
    cancelled_ = true;
    cpuHandle_.Cancel();
    imageDecodeBatch_.Cancel();
}

bool ImageSequenceLoadInstance::IsDone() const {
    return state_ == ImageSequenceLoadState::Ready || state_ == ImageSequenceLoadState::Failed || state_ == ImageSequenceLoadState::Cancelled;
}

bool ImageSequenceLoadInstance::IsReady() const {
    return state_ == ImageSequenceLoadState::Ready;
}

bool ImageSequenceLoadInstance::IsFailed() const {
    return state_ == ImageSequenceLoadState::Failed;
}

ImageSequenceLoadState ImageSequenceLoadInstance::GetState() const {
    return state_;
}

std::shared_ptr<ImageSequenceAsset> ImageSequenceLoadInstance::GetAsset() const {
    return asset_;
}

std::string ImageSequenceLoadInstance::GetError() const {
    return error_;
}

void ImageSequenceLoadInstance::BeginTextureUploads() {
    textureUploadBatch_.Reset(imageDecodeBatch_.GetDecodedImages(), uploadScheduler_, moonriver::Texture::GetSharedWhiteTexture());
}

bool ImageSequenceLoadInstance::AreTextureUploadsCompleted() const {
    return textureUploadBatch_.IsDone(uploadScheduler_);
}

void ImageSequenceLoadInstance::CompleteAsset() {
    std::shared_ptr<ImageSequenceAsset> asset(new ImageSequenceAsset());
    asset->name = desc_.name;
    asset->fps = desc_.fps;
    asset->loop = desc_.loop;
    asset->frames = textureUploadBatch_.GetTextures();
    asset_ = asset;
    state_ = ImageSequenceLoadState::Ready;
}

void ImageSequenceLoadInstance::Fail(const std::string& error) {
    error_ = error;
    state_ = ImageSequenceLoadState::Failed;
    std::printf("[ImageSequence] Load failed: %s\n", error.c_str());
}

} // namespace ImagePipeline
