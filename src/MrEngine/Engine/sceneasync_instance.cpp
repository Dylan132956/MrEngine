#include "sceneasync_instance.h"

#include "gltfasync_loader.h"
#include "graphics/Texture.h"
#include "graphics/Material.h"
#include "graphics/Shader.h"
#include "graphics/Mesh.h"
#include "graphics/MeshRenderer.h"
#include "graphics/SkinnedMeshRenderer.h"
#include "graphics/Camera.h"
#include "Entity.h"
#include "Scene.h"
#include "Transform.h"
#include "math/Mathf.h"
#include "memory/ByteBuffer.h"

#include <cstddef>
#include <cstdio>
#include <map>
#include <sstream>

namespace moonriver {

static void SetMaterialTexture(const std::shared_ptr<Material>& material,
                               const std::vector<std::shared_ptr<Texture> >& textures,
                               Material::TEXTURE_ID id,
                               const char* uniformName,
                               const std::shared_ptr<Texture>& fallback) {
    const int textureIndex = material->TextureIds[id];
    if (textureIndex >= 0 && static_cast<std::size_t>(textureIndex) < textures.size() && textures[textureIndex]) {
        material->SetTexture(uniformName, textures[textureIndex]);
    } else {
        material->SetTexture(uniformName, fallback);
    }
}

typedef std::pair<int, std::size_t> GLTFMeshPrimitiveKey;
typedef std::map<GLTFMeshPrimitiveKey, std::shared_ptr<Mesh> > GLTFMeshCache;

static std::shared_ptr<Entity> BuildNodeRecursive(const std::shared_ptr<SceneAsyncCpuAsset>& asset,
                                                  std::size_t nodeIndex,
                                                  const std::shared_ptr<Entity>& parent,
                                                  std::vector<std::shared_ptr<Entity> >& nodes,
                                                  const std::vector<std::shared_ptr<Material> >& materials,
                                                  GLTFMeshCache& meshCache) {
    if (!asset || nodeIndex >= asset->nodes.size()) return std::shared_ptr<Entity>();
    const GLTFAsyncNodeCpu& node = asset->nodes[nodeIndex];
    std::string name = node.name.empty() ? std::string("node_") + std::to_string(nodeIndex) : node.name;
    std::shared_ptr<Entity> entity = Entity::Create(name);
    nodes[nodeIndex] = entity;

    if (parent) entity->GetTransform()->SetParent(parent->GetTransform());
    entity->GetTransform()->SetLocalPosition(node.translation);
    entity->GetTransform()->SetLocalRotation(node.rotation);
    entity->GetTransform()->SetLocalScale(node.scale);
    if (node.hasMatrix) {
        Vector3 t, r, s;
        node.matrix.Decompose(t, r, s);
        entity->GetTransform()->SetLocalPosition(t);
        entity->GetTransform()->SetLocalRotation(Quaternion::Euler(r.x * Mathf::Rad2Deg, r.y * Mathf::Rad2Deg, r.z * Mathf::Rad2Deg));
        entity->GetTransform()->SetLocalScale(s);
    }

    for (std::size_t i = 0; i < node.children.size(); ++i) {
        BuildNodeRecursive(asset, static_cast<std::size_t>(node.children[i]), entity, nodes, materials, meshCache);
    }

    if (node.mesh >= 0 && static_cast<std::size_t>(node.mesh) < asset->meshes.size()) {
        GLTFAsyncMeshCpu& mesh = asset->meshes[node.mesh];
        for (std::size_t i = 0; i < mesh.primitives.size(); ++i) {
            GLTFAsyncPrimitiveCpu& primitive = mesh.primitives[i];
            const GLTFMeshPrimitiveKey cacheKey(node.mesh, i);
            GLTFMeshCache::iterator cached = meshCache.find(cacheKey);
            std::shared_ptr<Mesh> engineMesh;
            if (cached != meshCache.end()) {
                engineMesh = cached->second;
            } else {
                engineMesh.reset(new Mesh(std::move(primitive.vertices), std::move(primitive.indices)));
                meshCache[cacheKey] = engineMesh;
            }

            std::vector<std::shared_ptr<Material> > rendererMaterials;
            if (primitive.material >= 0 && static_cast<std::size_t>(primitive.material) < materials.size()) {
                rendererMaterials.push_back(materials[primitive.material]);
            } else {
                rendererMaterials.push_back(std::shared_ptr<Material>());
            }

            if (primitive.hasSkin) {
                std::shared_ptr<SkinnedMeshRenderer> renderer = entity->AddComponent<SkinnedMeshRenderer>();
                renderer->SetMesh(engineMesh);
                renderer->SetMaterials(rendererMaterials);
                renderer->EnableShaderKeyword("SKIN_ON");
                if (!rendererMaterials.empty() && rendererMaterials[0] &&
                    rendererMaterials[0]->Attribs.AlphaMode == Material::ALPHA_MODE_BLEND) {
                    renderer->EnableShaderKeyword("ALPHA_BLEND_ON");
                }
            } else {
                std::shared_ptr<MeshRenderer> renderer = entity->AddComponent<MeshRenderer>();
                renderer->SetMesh(engineMesh);
                renderer->SetMaterials(rendererMaterials);
                if (!rendererMaterials.empty() && rendererMaterials[0] &&
                    rendererMaterials[0]->Attribs.AlphaMode == Material::ALPHA_MODE_BLEND) {
                    renderer->EnableShaderKeyword("ALPHA_BLEND_ON");
                }
            }
        }
    }

    if (node.camera >= 0 && static_cast<std::size_t>(node.camera) < asset->cameras.size()) {
        const GLTFAsyncCameraCpu& cameraCpu = asset->cameras[node.camera];
        if (cameraCpu.valid) {
            std::shared_ptr<Camera> camera = entity->AddComponent<Camera>();
            camera->SetName(cameraCpu.name);
            camera->SetOrthographic(cameraCpu.orthographic);
            if (cameraCpu.orthographic) {
                camera->SetOrthographicSize(cameraCpu.ymag);
            } else {
                camera->SetAspect(cameraCpu.aspect);
                camera->SetFieldOfView(cameraCpu.yfov);
            }
            camera->SetNearClip(cameraCpu.znear);
            camera->SetFarClip(cameraCpu.zfar);
        }
    }

    return entity;
}

SceneAsyncLoadInstance::SceneAsyncLoadInstance(Scene* scene,
                                               AsyncLoad::AssetManager& assetManager,
                                               RenderUpload::FrameUploadScheduler& uploadScheduler,
                                               const std::string& fullPath)
    : scene_(scene),
      assetManager_(assetManager),
      uploadScheduler_(uploadScheduler),
      fullPath_(fullPath),
      state_(SceneAsyncLoadState::LoadingCpu),
      cancelled_(false) {}

void SceneAsyncLoadInstance::Start() {
    cpuHandle_ = assetManager_.LoadAsync<SceneAsyncCpuAsset>(fullPath_);
}

void SceneAsyncLoadInstance::Tick() {
    if (cancelled_) {
        state_ = SceneAsyncLoadState::Cancelled;
        return;
    }

    if (state_ == SceneAsyncLoadState::LoadingCpu) {
        if (!cpuHandle_.IsDone()) {
            return;
        }
        if (!cpuHandle_.IsSucceeded()) {
            std::printf("[GLTFAsync] CPU asset failed path=%s error=%s\n", fullPath_.c_str(), cpuHandle_.GetError().c_str());
            Fail(cpuHandle_.GetError());
            return;
        }

        cpuAsset_ = cpuHandle_.Get();
        BeginImageDecodes();

        state_ = imageDecodeBatch_.IsSubmitComplete() ? SceneAsyncLoadState::UploadingTextures : SceneAsyncLoadState::SubmittingImageDecodes;
        if (state_ == SceneAsyncLoadState::UploadingTextures) {
            BeginTextureUploads();
        }
        return;
    }

    if (state_ == SceneAsyncLoadState::SubmittingImageDecodes) {
        SubmitImageDecodeRequests();
        PollImageDecodes();
        if (imageDecodeBatch_.IsSubmitComplete()) {
            state_ = imageDecodeBatch_.IsDone() ? SceneAsyncLoadState::UploadingTextures : SceneAsyncLoadState::DecodingImages;
            if (state_ == SceneAsyncLoadState::UploadingTextures) {
                BeginTextureUploads();
            }
        }
        return;
    }

    if (state_ == SceneAsyncLoadState::DecodingImages) {
        PollImageDecodes();
        if (imageDecodeBatch_.IsDone()) {
            state_ = SceneAsyncLoadState::UploadingTextures;
            BeginTextureUploads();
        }
        return;
    }

    if (state_ == SceneAsyncLoadState::UploadingTextures) {
        if (AreTextureUploadsCompleted()) {
            state_ = SceneAsyncLoadState::AssemblingScene;
        } else {
            return;
        }
    }

    if (state_ == SceneAsyncLoadState::AssemblingScene) {
        AssembleScene();
        if (state_ != SceneAsyncLoadState::Failed) {
            state_ = SceneAsyncLoadState::Ready;
        }
    }
}

std::uint32_t SceneAsyncLoadInstance::SubmitTextureUploads(const RenderUpload::UploadBudget& budget) {
    const std::uint32_t submitted = textureUploadBatch_.SubmitFrame(uploadScheduler_, budget);
    textures_ = textureUploadBatch_.GetTextures();
    return submitted;
}

void SceneAsyncLoadInstance::Cancel() {
    cancelled_ = true;
    cpuHandle_.Cancel();
    imageDecodeBatch_.Cancel();
}

bool SceneAsyncLoadInstance::IsDone() const {
    return state_ == SceneAsyncLoadState::Ready || state_ == SceneAsyncLoadState::Failed || state_ == SceneAsyncLoadState::Cancelled;
}

bool SceneAsyncLoadInstance::IsReady() const {
    return state_ == SceneAsyncLoadState::Ready;
}

bool SceneAsyncLoadInstance::IsFailed() const {
    return state_ == SceneAsyncLoadState::Failed;
}

SceneAsyncLoadState SceneAsyncLoadInstance::GetState() const {
    return state_;
}

std::shared_ptr<Entity> SceneAsyncLoadInstance::GetRoot() const {
    return root_;
}

std::string SceneAsyncLoadInstance::GetError() const {
    return error_;
}

void SceneAsyncLoadInstance::Fail(const std::string& error) {
    error_ = error.empty() ? "glTF async load failed" : error;
    state_ = SceneAsyncLoadState::Failed;
}

void SceneAsyncLoadInstance::BeginImageDecodes() {
    std::vector<ImagePipeline::ImageSource> sources;
    if (cpuAsset_) {
        sources.reserve(cpuAsset_->images.size());
        for (std::size_t i = 0; i < cpuAsset_->images.size(); ++i) {
            const GLTFAsyncImageCpu& image = cpuAsset_->images[i];
            ImagePipeline::ImageSource source;
            source.id = fullPath_ + "#image/" + std::to_string(i);
            source.name = image.name.empty() ? std::string("gltf_image_") + std::to_string(i) : image.name;
            source.mimeType = image.mimeType;
            if (image.encodedBytesRef) {
                source.sharedBytes = std::shared_ptr<const std::vector<std::uint8_t> >(image.encodedBytesRef);
            } else {
                source.encodedBytes = image.encodedBytes;
            }
            sources.push_back(source);
        }
    }

    ImagePipeline::ImageDecodeBatchDesc desc;
    imageDecodeBatch_.Reset(sources, desc);
}

std::uint32_t SceneAsyncLoadInstance::SubmitImageDecodeRequests() {
    return imageDecodeBatch_.Submit(assetManager_);
}

void SceneAsyncLoadInstance::PollImageDecodes() {
    imageDecodeBatch_.Poll();
}

void SceneAsyncLoadInstance::BeginTextureUploads() {
    if (!cpuAsset_) return;

    const std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> >& decodedImages = imageDecodeBatch_.GetDecodedImages();
    std::vector<std::shared_ptr<ImageDecode::DecodedImageAsset> > textureImages;
    textureImages.assign(cpuAsset_->textures.size(), std::shared_ptr<ImageDecode::DecodedImageAsset>());

    for (std::size_t i = 0; i < cpuAsset_->textures.size(); ++i) {
        const GLTFAsyncTextureCpu& texture = cpuAsset_->textures[i];
        if (texture.sourceImage >= 0 && static_cast<std::size_t>(texture.sourceImage) < decodedImages.size()) {
            textureImages[i] = decodedImages[texture.sourceImage];
        }
    }

    textureUploadBatch_.Reset(textureImages, uploadScheduler_, Texture::GetSharedWhiteTexture());
    textures_ = textureUploadBatch_.GetTextures();
}

bool SceneAsyncLoadInstance::AreTextureUploadsCompleted() const {
    return textureUploadBatch_.IsDone(uploadScheduler_);
}

void SceneAsyncLoadInstance::AssembleScene() {
    if (!cpuAsset_) {
        Fail("Missing CPU glTF asset");
        return;
    }

    materials_.clear();
    for (std::size_t i = 0; i < cpuAsset_->materials.size(); ++i) {
        const GLTFAsyncMaterialCpu& materialCpu = cpuAsset_->materials[i];
        std::shared_ptr<Shader> shader = Shader::Find("standard");
        std::shared_ptr<Material> material(new Material(shader));
        material->Attribs = materialCpu.attribs;
        material->DoubleSided = materialCpu.doubleSided;
        material->TextureIds = materialCpu.textureIds;
        SetMaterialTexture(material, textures_, Material::TEXTURE_ID_BASE_COLOR,
            "SPIRV_Cross_Combinedg_ColorMapg_ColorSampler", Texture::GetSharedWhiteTexture());
        SetMaterialTexture(material, textures_, Material::TEXTURE_ID_PHYSICAL_DESC,
            "SPIRV_Cross_Combinedg_PhysicalDescriptorMapg_PhysicalDescriptorSampler", Texture::GetSharedGreenTexture());
        SetMaterialTexture(material, textures_, Material::TEXTURE_ID_NORMAL_MAP,
            "SPIRV_Cross_Combinedg_NormalMapg_NormalSampler", Texture::GetSharedNormalTexture());
        SetMaterialTexture(material, textures_, Material::TEXTURE_ID_OCCLUSION,
            "SPIRV_Cross_Combinedg_AOMapg_AOSampler", Texture::GetSharedWhiteTexture());
        SetMaterialTexture(material, textures_, Material::TEXTURE_ID_EMISSIVE,
            "SPIRV_Cross_Combinedg_EmissiveMapg_EmissiveSampler", Texture::GetSharedBlackTexture());
        if (material->Attribs.AlphaMode == Material::ALPHA_MODE_BLEND)
        {
            material->SetQueue((int)Shader::Queue::Transparent);
        }
        materials_.push_back(material);
    }

    root_ = Entity::Create("gltf_async_root");
    nodes_.assign(cpuAsset_->nodes.size(), std::shared_ptr<Entity>());
    GLTFMeshCache meshCache;
    for (std::size_t i = 0; i < cpuAsset_->sceneRootNodes.size(); ++i) {
        const int nodeIndex = cpuAsset_->sceneRootNodes[i];
        if (nodeIndex >= 0) {
            BuildNodeRecursive(cpuAsset_, static_cast<std::size_t>(nodeIndex), root_, nodes_, materials_, meshCache);
        }
    }

    if (root_) {
        root_->AddEntityTreeInScene(scene_);
    } else {
        Fail("Failed to assemble glTF scene");
    }
}

bool SceneAsyncLoadHandle::IsDone() const {
    return !instance_ || instance_->IsDone();
}

bool SceneAsyncLoadHandle::IsReady() const {
    return instance_ && instance_->IsReady();
}

bool SceneAsyncLoadHandle::IsFailed() const {
    return instance_ && instance_->IsFailed();
}

SceneAsyncLoadState SceneAsyncLoadHandle::GetState() const {
    return instance_ ? instance_->GetState() : SceneAsyncLoadState::Failed;
}

std::shared_ptr<Entity> SceneAsyncLoadHandle::GetRoot() const {
    return instance_ ? instance_->GetRoot() : std::shared_ptr<Entity>();
}

std::string SceneAsyncLoadHandle::GetError() const {
    return instance_ ? instance_->GetError() : std::string("glTF async load instance is empty");
}

} // namespace moonriver
