#pragma once

#include "asset/AssetManager.h"
#include "asset/image/ImageDecodeBatch.h"
#include "asset/image/TextureUploadBatch.h"
#include "sceneasync.h"

#include <memory>
#include <string>
#include <cstdint>
#include <vector>

namespace moonriver {

class Scene;
class Texture;
class Material;
class Mesh;

class SceneAsyncLoadInstance : public std::enable_shared_from_this<SceneAsyncLoadInstance> {
public:
    SceneAsyncLoadInstance(Scene* scene,
                           AsyncLoad::AssetManager& assetManager,
                           RenderUpload::FrameUploadScheduler& uploadScheduler,
                           const std::string& fullPath);

    void Start();
    void Tick();
    std::uint32_t SubmitTextureUploads(const RenderUpload::UploadBudget& budget);
    void Cancel();

    bool IsDone() const;
    bool IsReady() const;
    bool IsFailed() const;
    SceneAsyncLoadState GetState() const;
    std::shared_ptr<Entity> GetRoot() const;
    std::string GetError() const;

private:
    void BeginImageDecodes();
    std::uint32_t SubmitImageDecodeRequests();
    void PollImageDecodes();
    void BeginTextureUploads();
    bool AreTextureUploadsCompleted() const;
    void AssembleScene();
    void Fail(const std::string& error);

    Scene* scene_;
    AsyncLoad::AssetManager& assetManager_;
    RenderUpload::FrameUploadScheduler& uploadScheduler_;
    std::string fullPath_;
    SceneAsyncLoadState state_;
    std::string error_;
    AsyncLoad::AssetHandle<SceneAsyncCpuAsset> cpuHandle_;
    std::shared_ptr<SceneAsyncCpuAsset> cpuAsset_;
    ImagePipeline::ImageDecodeBatch imageDecodeBatch_;
    ImagePipeline::TextureUploadBatch textureUploadBatch_;
    std::vector<std::shared_ptr<Texture> > textures_;
    std::vector<std::shared_ptr<Material> > materials_;
    std::vector<std::shared_ptr<Entity> > nodes_;
    std::shared_ptr<Entity> root_;
    bool cancelled_;
};

} // namespace moonriver
