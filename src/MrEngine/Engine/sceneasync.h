#pragma once

#include "asset/Asset.h"
#include "graphics/Material.h"
#include "graphics/Mesh.h"
#include "graphics/Texture.h"
#include "graphics/Camera.h"
#include "animation/Animation.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace moonriver {

class Entity;
class SceneAsyncLoadInstance;

enum class SceneAsyncLoadState {
    LoadingCpu,
    SubmittingImageDecodes,
    DecodingImages,
    UploadingTextures,
    AssemblingScene,
    Ready,
    Failed,
    Cancelled
};

struct SceneAsyncSamplerCpu {
    FilterMode filterMode;
    SamplerAddressMode wrapMode;

    SceneAsyncSamplerCpu()
        : filterMode(FilterMode::Linear), wrapMode(SamplerAddressMode::Repeat) {}
};

struct SceneAsyncImageCpu {
    std::string name;
    std::string mimeType;
    std::vector<std::uint8_t> encodedBytes;
    std::shared_ptr<std::vector<std::uint8_t> > encodedBytesRef;
};

struct SceneAsyncTextureCpu {
    int sourceImage;
    int sampler;

    SceneAsyncTextureCpu() : sourceImage(-1), sampler(-1) {}
};

struct SceneAsyncMaterialCpu {
    std::string name;
    Material::ShaderAttribs attribs;
    bool doubleSided;
    std::array<int, Material::TEXTURE_ID_NUM_TEXTURES> textureIds;

    SceneAsyncMaterialCpu() : doubleSided(false) {
        textureIds.fill(-1);
    }
};

struct SceneAsyncPrimitiveCpu {
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    int material;
    bool hasSkin;

    SceneAsyncPrimitiveCpu() : material(-1), hasSkin(false) {}
};

struct SceneAsyncMeshCpu {
    std::string name;
    std::vector<SceneAsyncPrimitiveCpu> primitives;
};

struct SceneAsyncCameraCpu {
    std::string name;
    bool valid;
    bool orthographic;
    float aspect;
    float yfov;
    float znear;
    float zfar;
    float ymag;

    SceneAsyncCameraCpu()
        : valid(false), orthographic(false), aspect(0.0f), yfov(0.0f), znear(0.0f), zfar(0.0f), ymag(0.0f) {}
};

struct SceneAsyncNodeCpu {
    std::string name;
    int mesh;
    int skin;
    int camera;
    std::vector<int> children;
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
    bool hasMatrix;
    Matrix4x4 matrix;

    SceneAsyncNodeCpu()
        : mesh(-1), skin(-1), camera(-1), translation(0.0f, 0.0f, 0.0f),
          rotation(Quaternion::Identity()), scale(1.0f, 1.0f, 1.0f), hasMatrix(false) {}
};

struct SceneAsyncSkinCpu {
    std::string name;
    int skeletonRoot;
    std::vector<int> joints;
    std::vector<Matrix4x4> inverseBindMatrices;

    SceneAsyncSkinCpu() : skeletonRoot(-1) {}
};

struct SceneAsyncAnimationCpu {
    std::string name;
    std::vector<std::shared_ptr<AnimationClip> > clips;
};

class SceneAsyncCpuAsset : public AsyncLoad::Asset {
public:
    std::string sourcePath;
    std::string warnings;
    int defaultScene;
    std::vector<int> sceneRootNodes;
    std::vector<SceneAsyncSamplerCpu> samplers;
    std::vector<SceneAsyncImageCpu> images;
    std::vector<SceneAsyncTextureCpu> textures;
    std::vector<SceneAsyncMaterialCpu> materials;
    std::vector<SceneAsyncMeshCpu> meshes;
    std::vector<SceneAsyncNodeCpu> nodes;
    std::vector<SceneAsyncCameraCpu> cameras;
    std::vector<SceneAsyncSkinCpu> skins;
    std::vector<SceneAsyncAnimationCpu> animations;

    SceneAsyncCpuAsset() : defaultScene(0) {}
};

class SceneAsyncLoadHandle {
public:
    SceneAsyncLoadHandle() {}
    explicit SceneAsyncLoadHandle(const std::shared_ptr<SceneAsyncLoadInstance>& instance)
        : instance_(instance) {}

    bool IsDone() const;
    bool IsReady() const;
    bool IsFailed() const;
    SceneAsyncLoadState GetState() const;
    std::shared_ptr<Entity> GetRoot() const;
    std::string GetError() const;

private:
    std::shared_ptr<SceneAsyncLoadInstance> instance_;
};

typedef SceneAsyncLoadState GLTFAsyncState;
typedef SceneAsyncSamplerCpu GLTFAsyncSamplerCpu;
typedef SceneAsyncImageCpu GLTFAsyncImageCpu;
typedef SceneAsyncTextureCpu GLTFAsyncTextureCpu;
typedef SceneAsyncMaterialCpu GLTFAsyncMaterialCpu;
typedef SceneAsyncPrimitiveCpu GLTFAsyncPrimitiveCpu;
typedef SceneAsyncMeshCpu GLTFAsyncMeshCpu;
typedef SceneAsyncCameraCpu GLTFAsyncCameraCpu;
typedef SceneAsyncNodeCpu GLTFAsyncNodeCpu;
typedef SceneAsyncSkinCpu GLTFAsyncSkinCpu;
typedef SceneAsyncAnimationCpu GLTFAsyncAnimationCpu;
typedef SceneAsyncCpuAsset GLTFAsyncCpuAsset;
typedef SceneAsyncLoadInstance GLTFAsyncLoadInstance;
typedef SceneAsyncLoadHandle GLTFAsyncLoadHandle;

} // namespace moonriver
