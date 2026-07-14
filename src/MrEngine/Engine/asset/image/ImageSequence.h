#pragma once

#include "asset/Asset.h"
#include "asset/AssetLoader.h"
#include "asset/AssetManager.h"
#include "asset/FileSystem.h"
#include "asset/image/ImageDecodeBatch.h"
#include "asset/image/TextureUploadBatch.h"
#include "renderupload/FrameUploadScheduler.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace moonriver {
class Texture;
}

namespace ImagePipeline {

enum class ImageSequenceLoadState {
    LoadingSources,
    SubmittingImageDecodes,
    DecodingImages,
    UploadingTextures,
    Ready,
    Failed,
    Cancelled
};

struct ImageSequenceDesc {
    std::string name;
    std::vector<std::string> framePaths;
    float fps;
    bool loop;

    ImageSequenceDesc() : fps(30.0f), loop(true) {}
};

class ImageSequenceCpuAsset : public AsyncLoad::Asset {
public:
    ImageSequenceDesc desc;
    std::vector<ImageSource> sources;
};

class ImageSequenceCpuLoader : public AsyncLoad::TypedAssetLoader<ImageSequenceCpuAsset> {
public:
    bool CanLoad(const AsyncLoad::AssetLoadRequest& request) const override;
    AsyncLoad::LoadResult Load(const AsyncLoad::AssetLoadRequest& request,
                               AsyncLoad::IFileSystem& fileSystem,
                               const AsyncLoad::LoadContext& context) override;
};

class ImageSequenceAsset : public AsyncLoad::Asset {
public:
    ImageSequenceAsset();

    std::size_t GetFrameCount() const;
    std::shared_ptr<moonriver::Texture> GetFrame(std::size_t index) const;
    std::shared_ptr<moonriver::Texture> Sample(float timeSeconds) const;

    std::string name;
    float fps;
    bool loop;
    std::vector<std::shared_ptr<moonriver::Texture> > frames;
};

class ImageSequenceLoadInstance;

class ImageSequenceLoadHandle {
public:
    ImageSequenceLoadHandle();
    explicit ImageSequenceLoadHandle(const std::shared_ptr<ImageSequenceLoadInstance>& instance);

    bool IsDone() const;
    bool IsReady() const;
    bool IsFailed() const;
    ImageSequenceLoadState GetState() const;
    std::shared_ptr<ImageSequenceAsset> GetAsset() const;
    std::string GetError() const;

private:
    std::shared_ptr<ImageSequenceLoadInstance> instance_;
};

class ImageSequenceLoadInstance : public std::enable_shared_from_this<ImageSequenceLoadInstance> {
public:
    ImageSequenceLoadInstance(AsyncLoad::AssetManager& assetManager,
                              RenderUpload::FrameUploadScheduler& uploadScheduler,
                              const ImageSequenceDesc& desc);

    void Start();
    void Tick();
    std::uint32_t SubmitTextureUploads(const RenderUpload::UploadBudget& budget);
    void Cancel();

    bool IsDone() const;
    bool IsReady() const;
    bool IsFailed() const;
    ImageSequenceLoadState GetState() const;
    std::shared_ptr<ImageSequenceAsset> GetAsset() const;
    std::string GetError() const;

private:
    void BeginTextureUploads();
    bool AreTextureUploadsCompleted() const;
    void CompleteAsset();
    void Fail(const std::string& error);

    AsyncLoad::AssetManager& assetManager_;
    RenderUpload::FrameUploadScheduler& uploadScheduler_;
    ImageSequenceDesc desc_;
    ImageSequenceLoadState state_;
    bool cancelled_;
    std::string error_;
    AsyncLoad::AssetHandle<ImageSequenceCpuAsset> cpuHandle_;
    std::shared_ptr<ImageSequenceCpuAsset> cpuAsset_;
    ImageDecodeBatch imageDecodeBatch_;
    TextureUploadBatch textureUploadBatch_;
    std::shared_ptr<ImageSequenceAsset> asset_;
};

} // namespace ImagePipeline
