#include "gltfasync_loader.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <utility>

namespace moonriver {

static bool EndsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool LoadRawImage(tinygltf::Image* image,
                         const int,
                         std::string*,
                         std::string*,
                         int,
                         int,
                         const unsigned char* bytes,
                         int size,
                         void*) {
    image->image.resize(static_cast<std::size_t>(size));
    if (size > 0) {
        std::memcpy(&image->image[0], bytes, static_cast<std::size_t>(size));
    }
    image->width = 0;
    image->height = 0;
    image->component = 0;
    image->bits = 8;
    image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    return true;
}

static FilterMode ConvertFilter(int filter) {
    if (filter == TINYGLTF_TEXTURE_FILTER_NEAREST || filter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST || filter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
        return FilterMode::Nearest;
    }
    return FilterMode::Linear;
}

static SamplerAddressMode ConvertWrap(int wrap) {
    if (wrap == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) return SamplerAddressMode::ClampToEdge;
    if (wrap == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) return SamplerAddressMode::Mirror;
    return SamplerAddressMode::Repeat;
}

static Vector4 ReadColorFactor(const tinygltf::ParameterMap& params, const char* name, const Vector4& fallback) {
    tinygltf::ParameterMap::const_iterator it = params.find(name);
    if (it == params.end()) return fallback;
    const tinygltf::ColorValue values = it->second.ColorFactor();
    return Vector4(static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), static_cast<float>(values[3]));
}

static float ReadFactor(const tinygltf::ParameterMap& params, const char* name, float fallback) {
    tinygltf::ParameterMap::const_iterator it = params.find(name);
    return it == params.end() ? fallback : static_cast<float>(it->second.Factor());
}

static void ReadTextureParam(const tinygltf::ParameterMap& params,
                             const char* name,
                             Material::TEXTURE_ID textureId,
                             GLTFAsyncMaterialCpu& out) {
    tinygltf::ParameterMap::const_iterator it = params.find(name);
    if (it == params.end()) return;
    out.textureIds[textureId] = it->second.TextureIndex();
    const float uv = static_cast<float>(it->second.TextureTexCoord());
    if (textureId == Material::TEXTURE_ID_BASE_COLOR) out.attribs.BaseColorUVSelector = uv;
    if (textureId == Material::TEXTURE_ID_PHYSICAL_DESC) out.attribs.PhysicalDescriptorUVSelector = uv;
    if (textureId == Material::TEXTURE_ID_NORMAL_MAP) out.attribs.NormalUVSelector = uv;
    if (textureId == Material::TEXTURE_ID_OCCLUSION) out.attribs.OcclusionUVSelector = uv;
    if (textureId == Material::TEXTURE_ID_EMISSIVE) out.attribs.EmissiveUVSelector = uv;
}

static const unsigned char* AccessorData(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];
    return &buffer.data[accessor.byteOffset + view.byteOffset];
}

static bool ConvertMaterials(const tinygltf::Model& model, GLTFAsyncCpuAsset& asset) {
    for (std::size_t i = 0; i < model.materials.size(); ++i) {
        const tinygltf::Material& src = model.materials[i];
        GLTFAsyncMaterialCpu dst;
        dst.name = src.name;
        ReadTextureParam(src.values, "baseColorTexture", Material::TEXTURE_ID_BASE_COLOR, dst);
        ReadTextureParam(src.values, "metallicRoughnessTexture", Material::TEXTURE_ID_PHYSICAL_DESC, dst);
        ReadTextureParam(src.additionalValues, "normalTexture", Material::TEXTURE_ID_NORMAL_MAP, dst);
        ReadTextureParam(src.additionalValues, "occlusionTexture", Material::TEXTURE_ID_OCCLUSION, dst);
        ReadTextureParam(src.additionalValues, "emissiveTexture", Material::TEXTURE_ID_EMISSIVE, dst);
        dst.attribs.RoughnessFactor = ReadFactor(src.values, "roughnessFactor", dst.attribs.RoughnessFactor);
        dst.attribs.MetallicFactor = ReadFactor(src.values, "metallicFactor", dst.attribs.MetallicFactor);
        dst.attribs.BaseColorFactor = ReadColorFactor(src.values, "baseColorFactor", dst.attribs.BaseColorFactor);
        dst.attribs.EmissiveFactor = ReadColorFactor(src.additionalValues, "emissiveFactor", dst.attribs.EmissiveFactor);
        tinygltf::ParameterMap::const_iterator alpha = src.additionalValues.find("alphaMode");
        if (alpha != src.additionalValues.end()) {
            if (alpha->second.string_value == "BLEND") dst.attribs.AlphaMode = Material::ALPHA_MODE_BLEND;
            if (alpha->second.string_value == "MASK") dst.attribs.AlphaMode = Material::ALPHA_MODE_MASK;
        }
        dst.attribs.AlphaCutoff = ReadFactor(src.additionalValues, "alphaCutoff", dst.attribs.AlphaCutoff);
        tinygltf::ParameterMap::const_iterator doubleSided = src.additionalValues.find("doubleSided");
        if (doubleSided != src.additionalValues.end()) dst.doubleSided = doubleSided->second.bool_value;
        asset.materials.push_back(dst);
    }
    asset.materials.push_back(GLTFAsyncMaterialCpu());
    return true;
}

static bool ConvertMeshes(const tinygltf::Model& model, GLTFAsyncCpuAsset& asset, std::string& error) {
    for (std::size_t m = 0; m < model.meshes.size(); ++m) {
        const tinygltf::Mesh& srcMesh = model.meshes[m];
        GLTFAsyncMeshCpu dstMesh;
        dstMesh.name = srcMesh.name;
        for (std::size_t p = 0; p < srcMesh.primitives.size(); ++p) {
            const tinygltf::Primitive& primitive = srcMesh.primitives[p];
            std::map<std::string, int>::const_iterator posIt = primitive.attributes.find("POSITION");
            if (posIt == primitive.attributes.end()) {
                error = "glTF primitive missing POSITION";
                return false;
            }
            const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || posAccessor.type != TINYGLTF_TYPE_VEC3) {
                error = "POSITION must be float vec3";
                return false;
            }

            GLTFAsyncPrimitiveCpu dstPrim;
            dstPrim.material = primitive.material;
            const std::size_t vertexCount = posAccessor.count;
            dstPrim.vertices.resize(vertexCount);
            const float* pos = reinterpret_cast<const float*>(AccessorData(model, posAccessor));
            const int posStride = posAccessor.ByteStride(posView) / tinygltf::GetComponentSizeInBytes(posAccessor.componentType);

            const float* normal = nullptr;
            int normalStride = 0;
            std::map<std::string, int>::const_iterator normalIt = primitive.attributes.find("NORMAL");
            if (normalIt != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[normalIt->second];
                const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                normal = reinterpret_cast<const float*>(AccessorData(model, accessor));
                normalStride = accessor.ByteStride(view) / tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }

            const float* uv0 = nullptr;
            int uv0Stride = 0;
            std::map<std::string, int>::const_iterator uv0It = primitive.attributes.find("TEXCOORD_0");
            if (uv0It != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[uv0It->second];
                const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                uv0 = reinterpret_cast<const float*>(AccessorData(model, accessor));
                uv0Stride = accessor.ByteStride(view) / tinygltf::GetComponentSizeInBytes(accessor.componentType);
            }

            for (std::size_t v = 0; v < vertexCount; ++v) {
                Mesh::Vertex& vertex = dstPrim.vertices[v];
                vertex.vertex = Vector4(Vector3::MakeVector(pos + v * posStride), 1.0f);
                vertex.normal = normal ? Vector3::Normalize(Vector3::MakeVector(normal + v * normalStride)) : Vector3{};
                vertex.uv = uv0 ? Vector2::MakeVector(uv0 + v * uv0Stride) : Vector2{};
            }

            if (primitive.indices >= 0) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const void* data = AccessorData(model, accessor);
                dstPrim.indices.resize(accessor.count);
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    std::memcpy(dstPrim.indices.data(), data, sizeof(unsigned int) * accessor.count);
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const std::uint16_t* src = static_cast<const std::uint16_t*>(data);
                    for (std::size_t i = 0; i < accessor.count; ++i) dstPrim.indices[i] = src[i];
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const std::uint8_t* src = static_cast<const std::uint8_t*>(data);
                    for (std::size_t i = 0; i < accessor.count; ++i) dstPrim.indices[i] = src[i];
                } else {
                    error = "Unsupported index component type";
                    return false;
                }
            }
            dstMesh.primitives.push_back(std::move(dstPrim));
        }
        asset.meshes.push_back(std::move(dstMesh));
    }
    return true;
}

static void ConvertSkins(const tinygltf::Model& model, GLTFAsyncCpuAsset& asset) {
    for (std::size_t i = 0; i < model.skins.size(); ++i) {
        const tinygltf::Skin& src = model.skins[i];
        GLTFAsyncSkinCpu dst;
        dst.name = src.name;
        dst.skeletonRoot = src.skeleton;
        dst.joints = src.joints;
        if (src.inverseBindMatrices >= 0) {
            const tinygltf::Accessor& accessor = model.accessors[src.inverseBindMatrices];
            const unsigned char* data = AccessorData(model, accessor);
            dst.inverseBindMatrices.resize(accessor.count);
            std::memcpy(dst.inverseBindMatrices.data(), data, accessor.count * sizeof(Matrix4x4));
        }
        asset.skins.push_back(std::move(dst));
    }
}

static void ConvertNodesAndCameras(const tinygltf::Model& model, GLTFAsyncCpuAsset& asset) {
    asset.defaultScene = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (!model.scenes.empty()) asset.sceneRootNodes = model.scenes[asset.defaultScene].nodes;

    for (std::size_t i = 0; i < model.cameras.size(); ++i) {
        const tinygltf::Camera& src = model.cameras[i];
        GLTFAsyncCameraCpu dst;
        dst.valid = true;
        dst.name = src.name;
        dst.orthographic = src.type == "orthographic";
        if (dst.orthographic) {
            dst.ymag = static_cast<float>(src.orthographic.ymag);
            dst.znear = static_cast<float>(src.orthographic.znear);
            dst.zfar = static_cast<float>(src.orthographic.zfar);
        } else {
            dst.aspect = static_cast<float>(src.perspective.aspectRatio);
            dst.yfov = static_cast<float>(src.perspective.yfov);
            dst.znear = static_cast<float>(src.perspective.znear);
            dst.zfar = static_cast<float>(src.perspective.zfar);
        }
        asset.cameras.push_back(dst);
    }

    for (std::size_t i = 0; i < model.nodes.size(); ++i) {
        const tinygltf::Node& src = model.nodes[i];
        GLTFAsyncNodeCpu dst;
        dst.name = src.name;
        dst.mesh = src.mesh;
        dst.skin = src.skin;
        dst.camera = src.camera;
        dst.children = src.children;
        if (src.translation.size() == 3) dst.translation = Vector3(src.translation[0], src.translation[1], src.translation[2]);
        if (src.rotation.size() == 4) dst.rotation = Quaternion(src.rotation[0], src.rotation[1], src.rotation[2], src.rotation[3]);
        if (src.scale.size() == 3) dst.scale = Vector3(src.scale[0], src.scale[1], src.scale[2]);
        if (src.matrix.size() == 16) {
            dst.hasMatrix = true;
            dst.matrix = Matrix4x4::MakeMatrix(src.matrix.data()).Transpose();
        }
        asset.nodes.push_back(dst);
    }
}

bool GLTFAsyncCpuLoader::CanLoad(const AsyncLoad::AssetLoadRequest& request) const {
    return request.sourceKind == AsyncLoad::AssetSourceKind::Path && (EndsWith(request.path, ".gltf") || EndsWith(request.path, ".glb"));
}

AsyncLoad::LoadResult GLTFAsyncCpuLoader::Load(const AsyncLoad::AssetLoadRequest& request,
                                               AsyncLoad::IFileSystem&,
                                               const AsyncLoad::LoadContext& context) {
    if (context.IsCancelled()) return AsyncLoad::LoadResult::Failed("Cancelled");

    tinygltf::TinyGLTF loader;
    loader.SetImageLoader(LoadRawImage, nullptr);
    tinygltf::Model model;
    std::string error;
    std::string warning;
    const bool ok = EndsWith(request.path, ".glb")
        ? loader.LoadBinaryFromFile(&model, &error, &warning, request.path)
        : loader.LoadASCIIFromFile(&model, &error, &warning, request.path);
    if (!ok) return AsyncLoad::LoadResult::Failed(error.empty() ? "Failed to load glTF" : error);

    std::shared_ptr<SceneAsyncCpuAsset> asset(new SceneAsyncCpuAsset());
    asset->sourcePath = request.path;
    asset->warnings = warning;

    for (std::size_t i = 0; i < model.samplers.size(); ++i) {
        GLTFAsyncSamplerCpu sampler;
        sampler.filterMode = ConvertFilter(model.samplers[i].minFilter >= 0 ? model.samplers[i].minFilter : model.samplers[i].magFilter);
        sampler.wrapMode = ConvertWrap(model.samplers[i].wrapS);
        asset->samplers.push_back(sampler);
    }
    for (std::size_t i = 0; i < model.images.size(); ++i) {
        GLTFAsyncImageCpu image;
        image.name = model.images[i].name;
        image.mimeType = model.images[i].mimeType;
        image.encodedBytesRef.reset(new std::vector<std::uint8_t>());
        image.encodedBytesRef->swap(model.images[i].image);
        asset->images.push_back(std::move(image));
    }
    for (std::size_t i = 0; i < model.textures.size(); ++i) {
        GLTFAsyncTextureCpu texture;
        texture.sourceImage = model.textures[i].source;
        texture.sampler = model.textures[i].sampler;
        asset->textures.push_back(texture);
    }

    if (!ConvertMaterials(model, *asset)) return AsyncLoad::LoadResult::Failed("Failed to convert materials");
    if (!ConvertMeshes(model, *asset, error)) return AsyncLoad::LoadResult::Failed(error);
    ConvertNodesAndCameras(model, *asset);
    ConvertSkins(model, *asset);

    return AsyncLoad::LoadResult::Succeeded(asset);
}

} // namespace moonriver
