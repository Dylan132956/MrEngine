
#include "gltfview.h"
#include "Engine.h"
#include "private/backend/DriverApi.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include "graphics/Texture.h"
#include "graphics/Image.h"
#include "graphics/Material.h"
#include "Entity.h"
#include "graphics/Mesh.h"
#include "Transform.h"
#include "graphics/SkinnedMeshRenderer.h"
#include "graphics/MeshRenderer.h"
#include "graphics/Camera.h"
#include "animation/Animation.h"
#include "Scene.h"
#include "Debug.h"
#include <vector>


namespace moonriver
{
    struct MeshCache
    {
        std::shared_ptr<Mesh> mesh;
        std::vector<std::shared_ptr<Material>> materials;
    };

    struct LoadingCache
    {
        std::vector<MeshCache> meshes;
        std::vector<filament::backend::SamplerParams> textureSamplers;
        std::vector<std::shared_ptr<Texture>> textures;
        std::vector<std::shared_ptr<Material>> Materials;
        std::vector<std::shared_ptr<Entity>> nodes;
        std::vector<std::shared_ptr<Entity>> subMeshs;
    };


    filament::backend::SamplerWrapMode GetWrapMode(int32_t wrapMode)
    {
        switch (wrapMode)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return filament::backend::SamplerWrapMode::REPEAT;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return filament::backend::SamplerWrapMode::CLAMP_TO_EDGE;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return filament::backend::SamplerWrapMode::MIRRORED_REPEAT;
        default:
            return filament::backend::SamplerWrapMode::REPEAT;
        }
    }

    void LoadTextureSamplers(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        for (const tinygltf::Sampler& smpl : gltf_model.samplers)
        {
            filament::backend::SamplerParams SamDesc;
            switch (smpl.magFilter)
            {
            case TINYGLTF_TEXTURE_FILTER_NEAREST:
                SamDesc.filterMag = filament::backend::SamplerMagFilter::NEAREST;
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR:
                SamDesc.filterMag = filament::backend::SamplerMagFilter::LINEAR;
                break;
            default:
                SamDesc.filterMag = filament::backend::SamplerMagFilter::LINEAR;
            }

            switch (smpl.minFilter)
            {
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
                SamDesc.filterMin = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_NEAREST;
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                SamDesc.filterMin = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_NEAREST;
                break;
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
                SamDesc.filterMin = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                SamDesc.filterMin = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
                break;
            default:
                SamDesc.filterMin = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
            }

            SamDesc.wrapS = GetWrapMode(smpl.wrapS);
            SamDesc.wrapT = GetWrapMode(smpl.wrapT);

            cache->textureSamplers.push_back(SamDesc);
        }
    }

    bool LoadImageData(tinygltf::Image* gltf_image,
        const int            gltf_image_idx,
        std::string* error,
        std::string* warning,
        int                  req_width,
        int                  req_height,
        const unsigned char* image_data,
        int                  size,
        void* user_data)
    {
        (void)warning;

        const unsigned char* pData = image_data;

        if (size >= 3 && pData[0] == 0xFF && pData[1] == 0xD8 && pData[2] == 0xFF)
        {
            int k = 3;
        }

        if (size >= 8 &&
            pData[0] == 0x89 && pData[1] == 0x50 && pData[2] == 0x4E && pData[3] == 0x47 &&
            pData[4] == 0x0D && pData[5] == 0x0A && pData[6] == 0x1A && pData[7] == 0x0A)
        {
            int k = 3;
        }

        if (size >= 4 &&
            ((pData[0] == 0x49 && pData[1] == 0x20 && pData[2] == 0x49) ||
                (pData[0] == 0x49 && pData[1] == 0x49 && pData[2] == 0x2A && pData[3] == 0x00) ||
                (pData[0] == 0x4D && pData[1] == 0x4D && pData[2] == 0x00 && pData[3] == 0x2A) ||
                (pData[0] == 0x4D && pData[1] == 0x4D && pData[2] == 0x00 && pData[3] == 0x2B)))
        {
            int k = 3;
        }

        if (size >= 4 && pData[0] == 0x44 && pData[1] == 0x44 && pData[2] == 0x53 && pData[3] == 0x20)
        {
            int k = 3;
        }

        static constexpr unsigned char KTX10FileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
        static constexpr unsigned char KTX20FileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
        if (size >= 12 &&
            (memcmp(pData, KTX10FileIdentifier, sizeof(KTX10FileIdentifier)) == 0 ||
                memcmp(pData, KTX20FileIdentifier, sizeof(KTX20FileIdentifier)) == 0))

        {
            int k = 3;
        }

        //ByteBuffer buffer(const_cast<byte*>(pData), size / sizeof(byte));
        //std::shared_ptr<Image> pImage = Image::LoadFromMemory(buffer);
        //gltf_image->image.resize(pImage->data.Size());
        //memcpy(gltf_image->image.data(), pImage->data.Bytes(), pImage->data.Size() * sizeof(byte));

        gltf_image->image.resize(size / sizeof(byte));
        memcpy(gltf_image->image.data(), image_data, size);

        return true;
    }

    void LoadTextures(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        for (const tinygltf::Texture& gltf_tex : gltf_model.textures)
        {
            const tinygltf::Image& gltf_image = gltf_model.images[gltf_tex.source];
            ByteBuffer buffer(const_cast<byte*>(gltf_image.image.data()), gltf_image.image.size());
            //std::shared_ptr<Image> pImage = Image::LoadFromMemory(buffer);
            //pImage->EncodeToPNG("out.png");
            std::shared_ptr<Texture> pTexture = Texture::LoadTexture2DFromMemory(buffer, FilterMode::Linear, SamplerAddressMode::ClampToEdge, true);
            cache->textures.push_back(pTexture);
        }
    }

    void LoadMaterials(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        for (const tinygltf::Material& gltf_mat : gltf_model.materials)
        {
            std::shared_ptr<Material> Mat = std::make_shared<Material>(Shader::Find("standard"));
            struct TextureParameterInfo
            {
                const Material::TEXTURE_ID    TextureId;
                float& UVSelector;
                Vector4& UVScaleBias;
                float& Slice;
                const char* const             TextureName;
                const tinygltf::ParameterMap& Params;
            };

            std::array<TextureParameterInfo, 5> TextureParams =
            {
                TextureParameterInfo{Material::TEXTURE_ID_BASE_COLOR,    Mat->Attribs.BaseColorUVSelector,          Mat->Attribs.BaseColorUVScaleBias,          Mat->Attribs.BaseColorSlice,          "baseColorTexture",         gltf_mat.values},
                TextureParameterInfo{Material::TEXTURE_ID_PHYSICAL_DESC, Mat->Attribs.PhysicalDescriptorUVSelector, Mat->Attribs.PhysicalDescriptorUVScaleBias, Mat->Attribs.PhysicalDescriptorSlice, "metallicRoughnessTexture", gltf_mat.values},
                TextureParameterInfo{Material::TEXTURE_ID_NORMAL_MAP,    Mat->Attribs.NormalUVSelector,             Mat->Attribs.NormalUVScaleBias,             Mat->Attribs.NormalSlice,             "normalTexture",            gltf_mat.additionalValues},
                TextureParameterInfo{Material::TEXTURE_ID_OCCLUSION,     Mat->Attribs.OcclusionUVSelector,          Mat->Attribs.OcclusionUVScaleBias,          Mat->Attribs.OcclusionSlice,          "occlusionTexture",         gltf_mat.additionalValues},
                TextureParameterInfo{Material::TEXTURE_ID_EMISSIVE,      Mat->Attribs.EmissiveUVSelector,           Mat->Attribs.EmissiveUVScaleBias,           Mat->Attribs.EmissiveSlice,           "emissiveTexture",          gltf_mat.additionalValues}
            };

            for (const auto& Param : TextureParams)
            {
                auto tex_it = Param.Params.find(Param.TextureName);
                if (tex_it != Param.Params.end())
                {
                    Mat->TextureIds[Param.TextureId] = tex_it->second.TextureIndex();
                    Param.UVSelector = static_cast<float>(tex_it->second.TextureTexCoord());
                }
            }

            auto ReadFactor = [](float& Factor, const tinygltf::ParameterMap& Params, const char* Name) //
            {
                auto it = Params.find(Name);
                if (it != Params.end())
                {
                    Factor = static_cast<float>(it->second.Factor());
                }
            };

            ReadFactor(Mat->Attribs.RoughnessFactor, gltf_mat.values, "roughnessFactor");
            ReadFactor(Mat->Attribs.MetallicFactor, gltf_mat.values, "metallicFactor");

            auto ReadColorFactor = [](Vector4& Factor, const tinygltf::ParameterMap& Params, const char* Name) //
            {
                auto it = Params.find(Name);
                if (it != Params.end())
                {
                    Factor = Vector4::MakeVector(it->second.ColorFactor().data());
                }
            };

            ReadColorFactor(Mat->Attribs.BaseColorFactor, gltf_mat.values, "baseColorFactor");
            ReadColorFactor(Mat->Attribs.EmissiveFactor, gltf_mat.additionalValues, "emissiveFactor");
            {
                auto alpha_mode_it = gltf_mat.additionalValues.find("alphaMode");
                if (alpha_mode_it != gltf_mat.additionalValues.end())
                {
                    const tinygltf::Parameter& param = alpha_mode_it->second;
                    if (param.string_value == "BLEND")
                    {
                        Mat->Attribs.AlphaMode = Material::ALPHA_MODE_BLEND;
                    }
                    if (param.string_value == "MASK")
                    {
                        Mat->Attribs.AlphaMode = Material::ALPHA_MODE_MASK;
                        Mat->Attribs.AlphaCutoff = 0.5f;
                    }
                }
            }

            ReadFactor(Mat->Attribs.AlphaCutoff, gltf_mat.additionalValues, "alphaCutoff");

            {
                auto double_sided_it = gltf_mat.additionalValues.find("doubleSided");
                if (double_sided_it != gltf_mat.additionalValues.end())
                {
                    Mat->DoubleSided = double_sided_it->second.bool_value;
                }
            }

            Mat->Attribs.Workflow = Material::PBR_WORKFLOW_METALL_ROUGH;

            // Extensions
// @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers
            {
                auto ext_it = gltf_mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
                if (ext_it != gltf_mat.extensions.end())
                {
                    if (ext_it->second.Has("specularGlossinessTexture"))
                    {
                        auto index = ext_it->second.Get("specularGlossinessTexture").Get("index");
                        auto texCoordSet = ext_it->second.Get("specularGlossinessTexture").Get("texCoord");

                        Mat->TextureIds[Material::TEXTURE_ID_PHYSICAL_DESC] = index.Get<int>();
                        Mat->Attribs.PhysicalDescriptorUVSelector = static_cast<float>(texCoordSet.Get<int>());

                        Mat->Attribs.Workflow = Material::PBR_WORKFLOW_SPEC_GLOSS;
                    }

                    if (ext_it->second.Has("diffuseTexture"))
                    {
                        auto index = ext_it->second.Get("diffuseTexture").Get("index");
                        auto texCoordSet = ext_it->second.Get("diffuseTexture").Get("texCoord");

                        Mat->TextureIds[Material::TEXTURE_ID_BASE_COLOR] = index.Get<int>();
                        Mat->Attribs.BaseColorUVSelector = static_cast<float>(texCoordSet.Get<int>());
                    }

                    if (ext_it->second.Has("diffuseFactor"))
                    {
                        auto factor = ext_it->second.Get("diffuseFactor");
                        for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                        {
                            const auto val = factor.Get(i);
                            Mat->Attribs.BaseColorFactor[i] =
                                val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                        }
                    }

                    if (ext_it->second.Has("specularFactor"))
                    {
                        auto factor = ext_it->second.Get("specularFactor");
                        for (uint32_t i = 0; i < factor.ArrayLen(); i++)
                        {
                            const auto val = factor.Get(i);
                            Mat->Attribs.SpecularFactor[i] =
                                val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
                        }
                    }
                }
            }
            cache->Materials.push_back(Mat);
        }
        cache->Materials.push_back(std::shared_ptr<Material>());
    }

    void LoadNode(std::shared_ptr<Entity> parent, const tinygltf::Node& gltf_node, size_t nodeIndex, const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        std::shared_ptr<Entity> entity = Entity::Create(gltf_node.name);
        cache->nodes[nodeIndex] = entity;
        std::shared_ptr<Transform> transform = entity->GetTransform();
        if (parent) {
            transform->SetParent(parent->GetTransform());
        }
        transform->SetLocalPosition(Vector3(0.0, 0.0, 0.0));
        transform->SetLocalRotation(Quaternion::Identity());
        transform->SetLocalScale(Vector3(1.0, 1.0, 1.0));

        if (gltf_node.translation.size() == 3) {
            transform->SetLocalPosition(Vector3(gltf_node.translation[0], gltf_node.translation[1], gltf_node.translation[2]));
        }
        if (gltf_node.rotation.size() == 4) {
            transform->SetLocalRotation(Quaternion(gltf_node.rotation[0], gltf_node.rotation[1], gltf_node.rotation[2], gltf_node.rotation[3]));
        }
        if (gltf_node.scale.size() == 3) {
            transform->SetLocalScale(Vector3(gltf_node.scale[0], gltf_node.scale[1], gltf_node.scale[2]));
        }
        if (gltf_node.matrix.size() == 16) {
            Matrix4x4 matrix;
            memcpy(&matrix, &gltf_node.matrix, sizeof(Matrix4x4));
            Vector3 t, r, s;
            matrix.Decompose(t, r, s);
            transform->SetLocalPosition(t);
            transform->SetLocalRotation(Quaternion(r.x, r.y, r.z));
            transform->SetLocalScale(s);
        }
        if (gltf_node.children.size() > 0) {
            for (size_t i = 0; i < gltf_node.children.size(); ++i)
            {
                LoadNode(entity, gltf_model.nodes[gltf_node.children[i]], gltf_node.children[i], gltf_model, cache);
            }
        }
        
        if (gltf_node.mesh >= 0) {
            const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[gltf_node.mesh];
            for (size_t j = 0; j < gltf_mesh.primitives.size(); j++)
            {
                std::vector<Mesh::Vertex> vertices;
                std::vector<unsigned int> indices;

                const tinygltf::Primitive& primitive = gltf_mesh.primitives[j];
                uint32_t indexCount = 0;
                uint32_t vertexCount = 0;
                Vector3   PosMin;
                Vector3   PosMax;
                bool     hasSkin = false;
                bool     hasIndices = primitive.indices > -1;
                //vertices
                {

                    const float* bufferPos = nullptr;
                    const float* bufferColor = nullptr;
                    const float* bufferNormals = nullptr;
                    const float* bufferTangents = nullptr;
                    const float* bufferTexCoordSet0 = nullptr;
                    const float* bufferTexCoordSet1 = nullptr;
                    const uint8_t* bufferJoints8 = nullptr;
                    const uint16_t* bufferJoints16 = nullptr;
                    const float* bufferWeights = nullptr;

                    int posStride = -1;
                    int colorsStride = -1;
                    int normalsStride = -1;
                    int tangentsStride = -1;
                    int texCoordSet0Stride = -1;
                    int texCoordSet1Stride = -1;
                    int jointsStride = -1;
                    int weightsStride = -1;
                    {
                        auto position_it = primitive.attributes.find("POSITION");
                        assert(position_it != primitive.attributes.end());

                        const tinygltf::Accessor& posAccessor = gltf_model.accessors[position_it->second];
                        const tinygltf::BufferView& posView = gltf_model.bufferViews[posAccessor.bufferView];
                        assert(posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(posAccessor.type == TINYGLTF_TYPE_VEC3);

                        bufferPos = reinterpret_cast<const float*>(&(gltf_model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
                        PosMin =
                            Vector3 //
                        {
                            static_cast<float>(posAccessor.minValues[0]),
                            static_cast<float>(posAccessor.minValues[1]),
                            static_cast<float>(posAccessor.minValues[2]) //
                        };
                        PosMax =
                            Vector3 //
                        {
                            static_cast<float>(posAccessor.maxValues[0]),
                            static_cast<float>(posAccessor.maxValues[1]),
                            static_cast<float>(posAccessor.maxValues[2]) //
                        };
                        posStride = posAccessor.ByteStride(posView) / tinygltf::GetComponentSizeInBytes(posAccessor.componentType);
                        assert(posStride > 0);

                        vertexCount = static_cast<uint32_t>(posAccessor.count);
                    }

                    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& colorAccessor = gltf_model.accessors[primitive.attributes.find("COLOR_0")->second];
                        const tinygltf::BufferView& colorView = gltf_model.bufferViews[colorAccessor.bufferView];
                        assert(colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(colorAccessor.type == TINYGLTF_TYPE_VEC4);

                        bufferColor = reinterpret_cast<const float*>(&(gltf_model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
                        colorsStride = colorAccessor.ByteStride(colorView) / tinygltf::GetComponentSizeInBytes(colorAccessor.componentType);
                        assert(colorsStride > 0);
                    }

                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& normAccessor = gltf_model.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& normView = gltf_model.bufferViews[normAccessor.bufferView];
                        assert(normAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(normAccessor.type == TINYGLTF_TYPE_VEC3);

                        bufferNormals = reinterpret_cast<const float*>(&(gltf_model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                        normalsStride = normAccessor.ByteStride(normView) / tinygltf::GetComponentSizeInBytes(normAccessor.componentType);
                        assert(normalsStride > 0);
                    }

                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& tangentAccessor = gltf_model.accessors[primitive.attributes.find("TANGENT")->second];
                        const tinygltf::BufferView& tangentView = gltf_model.bufferViews[tangentAccessor.bufferView];
                        assert(tangentAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(tangentAccessor.type == TINYGLTF_TYPE_VEC4);

                        bufferTangents = reinterpret_cast<const float*>(&(gltf_model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
                        tangentsStride = tangentAccessor.ByteStride(tangentView) / tinygltf::GetComponentSizeInBytes(tangentAccessor.componentType);
                        assert(tangentsStride > 0);
                    }

                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& uvAccessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& uvView = gltf_model.bufferViews[uvAccessor.bufferView];
                        assert(uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(uvAccessor.type == TINYGLTF_TYPE_VEC2);

                        bufferTexCoordSet0 = reinterpret_cast<const float*>(&(gltf_model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                        texCoordSet0Stride = uvAccessor.ByteStride(uvView) / tinygltf::GetComponentSizeInBytes(uvAccessor.componentType);
                        assert(texCoordSet0Stride > 0);
                    }
                    if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& uvAccessor = gltf_model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                        const tinygltf::BufferView& uvView = gltf_model.bufferViews[uvAccessor.bufferView];
                        assert(uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(uvAccessor.type == TINYGLTF_TYPE_VEC2);

                        bufferTexCoordSet1 = reinterpret_cast<const float*>(&(gltf_model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                        texCoordSet1Stride = uvAccessor.ByteStride(uvView) / tinygltf::GetComponentSizeInBytes(uvAccessor.componentType);
                        assert(texCoordSet1Stride > 0);
                    }

                    // Skinning
                    // Joints
                    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& jointAccessor = gltf_model.accessors[primitive.attributes.find("JOINTS_0")->second];
                        const tinygltf::BufferView& jointView = gltf_model.bufferViews[jointAccessor.bufferView];
                        assert(jointAccessor.type == TINYGLTF_TYPE_VEC4);

                        const auto* bufferJoints = &(gltf_model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
                        switch (jointAccessor.componentType)
                        {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            bufferJoints16 = reinterpret_cast<const uint16_t*>(bufferJoints);
                            break;

                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            bufferJoints8 = reinterpret_cast<const uint8_t*>(bufferJoints);
                            break;

                        default:
                            Log("Joint component type is expected to be unsigned short or byte");
                            assert(0);
                        }

                        jointsStride = jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointAccessor.componentType);
                        assert(jointsStride > 0);
                    }

                    if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& weightsAccessor = gltf_model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                        const tinygltf::BufferView& weightsView = gltf_model.bufferViews[weightsAccessor.bufferView];
                        assert(weightsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        assert(weightsAccessor.type == TINYGLTF_TYPE_VEC4);

                        bufferWeights = reinterpret_cast<const float*>(&(gltf_model.buffers[weightsView.buffer].data[weightsAccessor.byteOffset + weightsView.byteOffset]));
                        weightsStride = weightsAccessor.ByteStride(weightsView) / tinygltf::GetComponentSizeInBytes(weightsAccessor.componentType);
                        assert(weightsStride > 0);
                    }

                    hasSkin = bufferWeights != nullptr && (bufferJoints8 != nullptr || bufferJoints16 != nullptr);

                    vertices.resize(vertexCount);
                    for (uint32_t v = 0; v < vertexCount; v++)
                    {
                        vertices[v].vertex = Vector4(Vector3::MakeVector(bufferPos + v * posStride), 1.0f);
                        vertices[v].color = bufferColor != nullptr ? Color::MakeVector(bufferColor + v * colorsStride) : Color{};
                        vertices[v].normal = bufferNormals != nullptr ? Vector3::Normalize(Vector3::MakeVector(bufferNormals + v * normalsStride)) : Vector3{};
                        vertices[v].tangent = bufferTangents != nullptr ? Vector4::MakeVector(bufferTangents + v * tangentsStride) : Vector4{};
                        vertices[v].uv = bufferTexCoordSet0 != nullptr ? Vector2::MakeVector(bufferTexCoordSet0 + v * texCoordSet0Stride) : Vector2{};
                        vertices[v].uv2 = bufferTexCoordSet1 != nullptr ? Vector2::MakeVector(bufferTexCoordSet1 + v * texCoordSet1Stride) : Vector2{};
                        if (hasSkin) {
                            vertices[v].bone_weights = bufferWeights != nullptr ? Vector4::MakeVector(bufferWeights + v * weightsStride) : Vector4{};
                            vertices[v].bone_indices = bufferJoints8 != nullptr ? Vector4::MakeVector(bufferJoints8 + v * jointsStride) : Vector4::MakeVector(bufferJoints16 + v * jointsStride);
                        }
                    }
                }

                if (hasIndices)
                {
                    const tinygltf::Accessor& accessor = gltf_model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                    const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = gltf_model.buffers[bufferView.buffer];

                    indexCount = static_cast<uint32_t>(accessor.count);

                    const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                    indices.resize(accessor.count);
                    switch (accessor.componentType)
                    {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    {
                        const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                        memcpy(indices.data(), buf, sizeof(uint32_t)* accessor.count);
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices[index] = buf[index];
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices[index] = buf[index];
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }
                }

                std::shared_ptr<Mesh> mesh = std::shared_ptr<Mesh>(new Mesh(std::move(vertices), std::move(indices)));
                std::shared_ptr<Entity> pEntity = Entity::Create("submesh");
                cache->subMeshs.push_back(pEntity);
                pEntity->GetTransform()->SetParent(entity->GetTransform());
                pEntity->GetTransform()->SetLocalPosition(Vector3(0, 0, 0));
                pEntity->GetTransform()->SetLocalRotation(Quaternion::Identity());
                pEntity->GetTransform()->SetLocalScale(Vector3(1, 1, 1));
                if (hasSkin)
                {
                    std::shared_ptr<SkinnedMeshRenderer> renderer = entity->AddComponent<SkinnedMeshRenderer>();
                    renderer->SetMesh(mesh);
                    std::vector<std::shared_ptr<Material>> vMaterial;
                    vMaterial.push_back(cache->Materials[primitive.material]);
                    renderer->SetMaterials(vMaterial);
                }
                else
                {
                    std::shared_ptr<MeshRenderer> renderer = entity->AddComponent<MeshRenderer>();
                    renderer->SetMesh(mesh);
                    std::vector<std::shared_ptr<Material>> vMaterial;
                    vMaterial.push_back(cache->Materials[primitive.material]);
                    renderer->SetMaterials(vMaterial);
                }

            }
        }

        // Node contains camera
        if (gltf_node.camera >= 0)
        {
            const auto& gltf_cam = gltf_model.cameras[gltf_node.camera];
            std::shared_ptr<Camera> pCamera = entity->AddComponent<Camera>();
            pCamera->SetName(gltf_cam.name);
            if (gltf_cam.type == "perspective")
            {
                pCamera->SetOrthographic(false);
                pCamera->SetAspect(static_cast<float>(gltf_cam.perspective.aspectRatio));
                pCamera->SetFieldOfView(static_cast<float>(gltf_cam.perspective.yfov));
                pCamera->SetNearClip(static_cast<float>(gltf_cam.perspective.znear));
                pCamera->SetFarClip(static_cast<float>(gltf_cam.perspective.zfar));
            }
            else if (gltf_cam.type == "orthographic")
            {
                pCamera->SetOrthographic(true);
                pCamera->SetOrthographicSize(static_cast<float>(gltf_cam.orthographic.ymag));
                pCamera->SetNearClip(static_cast<float>(gltf_cam.orthographic.znear));
                pCamera->SetFarClip(static_cast<float>(gltf_cam.orthographic.zfar));
            }
            else
            {
                Log("Unexpected camera type: ", gltf_cam.type);
            }
        }
    }

    std::shared_ptr<Entity> LoadNodes(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        const tinygltf::Scene& scene = gltf_model.scenes[gltf_model.defaultScene > -1 ? gltf_model.defaultScene : 0];
        std::shared_ptr<Entity> entity = Entity::Create(scene.name);
        for (size_t i = 0; i < scene.nodes.size(); i++)
        {
            const tinygltf::Node node = gltf_model.nodes[scene.nodes[i]];
            LoadNode(entity, node, scene.nodes[i], gltf_model, cache);
        }
        return entity;
    }

    void LoadSkin(const tinygltf::Model& gltf_model, LoadingCache* cache, const tinygltf::Node& gltf_node, std::shared_ptr<Entity> entity)
    {
        if (!entity)
        {
            return;
        }
        if (gltf_node.mesh < 0 || gltf_node.mesh >= gltf_model.meshes.size())
        {
            return;
        }
        if (gltf_node.skin < 0 || gltf_node.skin >= gltf_model.skins.size())
        {
            return;
        }
        const tinygltf::Skin& tfSkin = gltf_model.skins[gltf_node.skin];
        int rootIndex = tfSkin.skeleton;
        if (rootIndex < 0)
        {
            rootIndex = 0;
        }
        std::shared_ptr<Entity> root = cache->nodes[rootIndex];

        std::vector<std::shared_ptr<Transform>> bones;
        for (int i = 0; i < tfSkin.joints.size(); ++i)
        {
            bones.push_back(cache->nodes[tfSkin.joints[i]]->GetTransform());
        }
        std::shared_ptr<SkinnedMeshRenderer> renderer = entity->GetComponent<SkinnedMeshRenderer>();
        renderer->SetBonesRoot(root->GetTransform());
        renderer->SetBones(bones);
        if (tfSkin.inverseBindMatrices > -1)
        {
            std::vector<Matrix4x4> inverseBindMatrices;
            const tinygltf::Accessor& accessor = gltf_model.accessors[tfSkin.inverseBindMatrices];
            const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = gltf_model.buffers[bufferView.buffer];
            inverseBindMatrices.resize(accessor.count);
            memcpy(inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(Matrix4x4));
            renderer->GetMesh()->SetBindposes(std::move(inverseBindMatrices));
        }
    }

    void LoadSkins(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        for (size_t i = 0; i < gltf_model.nodes.size(); ++i)
        {
            if (gltf_model.nodes[i].skin >= 0 && gltf_model.nodes[i].mesh >= 0)
            {
                LoadSkin(gltf_model, cache, gltf_model.nodes[i], cache->nodes[i]);
            }
        }
    }

    std::shared_ptr<Transform> GetSameParent(std::shared_ptr<Transform> entity1, std::shared_ptr<Transform> entity2)
    {
        // entity1 is entity2
        if (entity1 == entity2)
        {
            return entity1;
        }
        if (!entity1 || !entity2)
        {
            return nullptr;
        }

        std::shared_ptr<Transform> sameParent = nullptr;

        std::shared_ptr<Transform> parent1 = entity1;
        while (parent1)
        {
            std::shared_ptr<Transform> parent2 = entity2;
            while (parent2)
            {
                if (parent1 == parent2)
                {
                    sameParent = parent1;
                    break;
                }
                parent2 = parent2->GetParent();
            }
            if (sameParent)
            {
                break;
            }
            parent1 = parent1->GetParent();
        }

        return sameParent;
    }

    void LoadAnimations(const tinygltf::Model& gltf_model, LoadingCache* cache)
    {
        for (size_t animationIndex = 0; animationIndex < gltf_model.animations.size(); ++animationIndex)
        {
            const tinygltf::Animation& gltf_anim = gltf_model.animations[animationIndex];
            std::shared_ptr<Transform> topmostParent = nullptr;
            for (auto& source : gltf_anim.channels)
            {
                std::shared_ptr<Entity> targetEntity(nullptr);
                if (source.target_node >= 0 && source.target_node < cache->nodes.size())
                {
                    targetEntity = cache->nodes[source.target_node];
                }
                if (targetEntity == nullptr) {
                    continue;
                }
                if (topmostParent == nullptr) 
                {
                    topmostParent = targetEntity->GetTransform();
                }
                else
                {
                    topmostParent = GetSameParent(topmostParent, targetEntity->GetTransform());
                }
                if (topmostParent == nullptr)
                {
                    break;
                }
            }

            if (topmostParent == nullptr)
            {
                Log("glTF Loader: Animation(index:%s) no same parent node.", animationIndex);
                return;
            }

            std::shared_ptr<Animation> animationCom = topmostParent->GetEntity()->GetComponent<Animation>();
            if (animationCom == nullptr)
            {
                animationCom = topmostParent->GetEntity()->AddComponent<Animation>();
            }

            std::string clipName = gltf_anim.name;
            if (clipName.empty())
            {
                char name[64];
                sprintf(name, "DefaultClip_%d", animationIndex);
                clipName = name;
            }
            std::shared_ptr<AnimationClip> clip = std::make_shared<AnimationClip>();

            for (auto& source : gltf_anim.channels)
            {
                const tinygltf::AnimationSampler& samp = gltf_anim.samplers[source.sampler];
                // times
                std::vector<float> times;
                {
                    const tinygltf::Accessor& accessor = gltf_model.accessors[samp.input];
                    const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = gltf_model.buffers[bufferView.buffer];
                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                    const float* buf = static_cast<const float*>(dataPtr);
                    times.resize(accessor.count);
                    memcpy(times.data(), buf, sizeof(float) * accessor.count);
                }

                // Read sampler output T/R/S values
                {
                    const tinygltf::Accessor& accessor = gltf_model.accessors[samp.output];
                    const tinygltf::BufferView& bufferView = gltf_model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = gltf_model.buffers[bufferView.buffer];

                    assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                    const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

                    if (source.target_path == "rotation")
                    {
                        assert(accessor.type == TINYGLTF_TYPE_VEC4);
                        std::vector<Quaternion> rotations;
                        const Vector4* buf = static_cast<const Vector4*>(dataPtr);
                        rotations.resize(accessor.count);
                        memcpy(rotations.data(), buf, sizeof(Vector4) * accessor.count);

                        AnimationCurveProperty propertyX;
                        propertyX.type = AnimationCurvePropertyType::LocalRotationX;
                        AnimationCurveProperty propertyY;
                        propertyY.type = AnimationCurvePropertyType::LocalRotationY;
                        AnimationCurveProperty propertyZ;
                        propertyZ.type = AnimationCurvePropertyType::LocalRotationZ;
                        AnimationCurveProperty propertyW;
                        propertyW.type = AnimationCurvePropertyType::LocalRotationW;

                        if (samp.interpolation == "CUBICSPLINE")
                        {
                            assert(times.size() == rotations.size() / 3);

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], rotations[j * 3 + 1].x, rotations[j * 3 + 0].x, rotations[j * 3 + 2].x);
                                propertyY.curve.AddKey(times[j], rotations[j * 3 + 1].y, rotations[j * 3 + 0].y, rotations[j * 3 + 2].y);
                                propertyZ.curve.AddKey(times[j], rotations[j * 3 + 1].z, rotations[j * 3 + 0].z, rotations[j * 3 + 2].z);
                                propertyW.curve.AddKey(times[j], rotations[j * 3 + 1].w, rotations[j * 3 + 0].w, rotations[j * 3 + 2].w);
                            }
                        }
                        else
                        {
                            assert(times.size() == rotations.size());

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], rotations[j].x, 0.0, 0.0);
                                propertyY.curve.AddKey(times[j], rotations[j].y, 0.0, 0.0);
                                propertyZ.curve.AddKey(times[j], rotations[j].z, 0.0, 0.0);
                                propertyW.curve.AddKey(times[j], rotations[j].w, 0.0, 0.0);
                            }
                        }

                        AnimationCurveWrapper new_path_curve;
                        new_path_curve.properties.push_back(propertyX);
                        new_path_curve.properties.push_back(propertyY);
                        new_path_curve.properties.push_back(propertyZ);
                        new_path_curve.properties.push_back(propertyW);

                        clip->curves.push_back(new_path_curve);
                    }
                    else if (source.target_path == "translation")
                    {
                        assert(accessor.type == TINYGLTF_TYPE_VEC3);
                        std::vector<Vector3> positions;
                        const Vector3* buf = static_cast<const Vector3*>(dataPtr);
                        positions.resize(accessor.count);
                        memcpy(positions.data(), buf, sizeof(Vector3) * accessor.count);

                        AnimationCurveProperty propertyX;
                        propertyX.type = AnimationCurvePropertyType::LocalPositionX;
                        AnimationCurveProperty propertyY;
                        propertyY.type = AnimationCurvePropertyType::LocalPositionY;
                        AnimationCurveProperty propertyZ;
                        propertyZ.type = AnimationCurvePropertyType::LocalPositionZ;

                        if (samp.interpolation == "CUBICSPLINE")
                        {
                            assert(times.size() == positions.size() / 3);

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], positions[j * 3 + 1].x, positions[j * 3 + 0].x, positions[j * 3 + 2].x);
                                propertyY.curve.AddKey(times[j], positions[j * 3 + 1].y, positions[j * 3 + 0].y, positions[j * 3 + 2].y);
                                propertyZ.curve.AddKey(times[j], positions[j * 3 + 1].z, positions[j * 3 + 0].z, positions[j * 3 + 2].z);
                            }
                        }
                        else
                        {
                            assert(times.size() == positions.size());

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], positions[j].x, 0.0f, 0.0f);
                                propertyY.curve.AddKey(times[j], positions[j].y, 0.0f, 0.0f);
                                propertyZ.curve.AddKey(times[j], positions[j].z, 0.0f, 0.0f);
                            }
                        }

                        AnimationCurveWrapper new_path_curve;
                        new_path_curve.properties.push_back(propertyX);
                        new_path_curve.properties.push_back(propertyY);
                        new_path_curve.properties.push_back(propertyZ);
                        clip->curves.push_back(new_path_curve);
                    }
                    else if (source.target_path == "scale")
                    {
                        assert(accessor.type == TINYGLTF_TYPE_VEC3);
                        std::vector<Vector3> scales;
                        const Vector3* buf = static_cast<const Vector3*>(dataPtr);
                        scales.resize(accessor.count);
                        memcpy(scales.data(), buf, sizeof(Vector3)* accessor.count);

                        AnimationCurveProperty propertyX;
                        propertyX.type = AnimationCurvePropertyType::LocalScaleX;
                        AnimationCurveProperty propertyY;
                        propertyY.type = AnimationCurvePropertyType::LocalScaleY;
                        AnimationCurveProperty propertyZ;
                        propertyZ.type = AnimationCurvePropertyType::LocalScaleZ;

                        if (samp.interpolation == "CUBICSPLINE")
                        {
                            assert(times.size() == scales.size() / 3);

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], scales[j * 3 + 1].x, scales[j * 3 + 0].x, scales[j * 3 + 2].x);
                                propertyY.curve.AddKey(times[j], scales[j * 3 + 1].y, scales[j * 3 + 0].y, scales[j * 3 + 2].y);
                                propertyZ.curve.AddKey(times[j], scales[j * 3 + 1].z, scales[j * 3 + 0].z, scales[j * 3 + 2].z);
                            }
                        }
                        else
                        {
                            assert(times.size() == scales.size());

                            for (int j = 0; j < times.size(); ++j)
                            {
                                propertyX.curve.AddKey(times[j], scales[j].x, 0.0f, 0.0f);
                                propertyY.curve.AddKey(times[j], scales[j].y, 0.0f, 0.0f);
                                propertyZ.curve.AddKey(times[j], scales[j].z, 0.0f, 0.0f);
                            }
                        }

                        AnimationCurveWrapper new_path_curve;
                        new_path_curve.properties.push_back(propertyX);
                        new_path_curve.properties.push_back(propertyY);
                        new_path_curve.properties.push_back(propertyZ);
                        clip->curves.push_back(new_path_curve);

                    }
                    else if (source.target_path == "weights")
                    {
                        Log("Weights not yet supported, skipping channel");
                        continue;
                    }

                }
            }

            for (auto& samp : gltf_anim.samplers)
            {

            }

        }
    }

    std::shared_ptr<Entity> Scene::LoadGLTF(const char* name)
    {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        loader.SetImageLoader(LoadImageData, nullptr);

        //std::string asset_path = Engine::Instance()->GetDataPath() + "/model/DamagedHelmet/DamagedHelmet.gltf";
        //std::string asset_path = Engine::Instance()->GetDataPath() + "/model/Sponza/Sponza.gltf";
        std::string asset_path = Engine::Instance()->GetDataPath() + name;
        std::string input_filename = asset_path;

        std::string ext = tinygltf::GetFilePathExtension(input_filename);

        bool ret = false;
        if (ext.compare("glb") == 0) {
            // assume binary glTF.
            ret =
                loader.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
        }
        else {
            // assume ascii glTF.
            ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
        }
        if (!warn.empty()) {
            printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            printf("ERR: %s\n", err.c_str());
        }
        if (!ret) {
            printf("Failed to load .glTF : %s\n", input_filename.c_str());
            exit(-1);
        }

        LoadingCache* cache = new LoadingCache();
        cache->nodes.resize(model.nodes.size());

        LoadTextureSamplers(model, cache);
        LoadTextures(model, cache);
        LoadMaterials(model, cache);

        std::shared_ptr<Entity> root = LoadNodes(model, cache);

        LoadAnimations(model, cache);

        LoadSkins(model, cache);

        root->AddEntityTreeInScene(this);

        delete cache;

        return root;
    }

    gltfview::gltfview()
    {

    }

    gltfview::~gltfview()
    {

    }
}