#pragma once
#include "Object.h"
#include "private/backend/DriverApi.h"
#include "memory/ByteBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "graphics/Color.h"
#include <map>
#include "memory/Memory.h"
#include "math/Rect.h"

namespace moonriver
{
    // per view uniforms, set by camera
    struct ViewUniforms
    {
        static constexpr const char* VIEW_MATRIX = "u_view_matrix";
        static constexpr const char* PROJECTION_MATRIX = "u_projection_matrix";
        static constexpr const char* CAMERA_POS = "u_camera_pos";
        static constexpr const char* TIME = "u_time";

        Matrix4x4 view_matrix;
        Matrix4x4 projection_matrix;
        Vector4 camera_pos;
        Vector4 time;
    };
    // per renderer uniforms, set by renderer
    struct RendererUniforms
    {
        static constexpr const char* MODEL_MATRIX = "u_model_matrix";
        static constexpr const char* BOUNDS_MATRIX = "u_bounds_matrix";
        static constexpr const char* BOUNDS_COLOR = "u_bounds_color";
        static constexpr const char* LIGHTMAP_SCALE_OFFSET = "u_lightmap_scale_offset";
        static constexpr const char* LIGHTMAP_INDEX = "u_lightmap_index";

        Matrix4x4 model_matrix;
        Matrix4x4 bounds_matrix;
        Color bounds_color;
        Vector4 lightmap_scale_offset;
        Vector4 lightmap_index; // in x
    };
    // per light uniforms, set by light
    struct LightFragmentUniforms
    {
        static constexpr const char* AMBIENT_COLOR = "u_ambient_color";
        static constexpr const char* LIGHT_POS = "u_light_pos";
        static constexpr const char* LIGHT_COLOR = "u_light_color";
        static constexpr const char* LIGHT_ATTEN = "u_light_atten";
        static constexpr const char* SPOT_LIGHT_DIR = "u_spot_light_dir";
        static constexpr const char* SHADOW_PARAMS = "u_shadow_params";

        Color ambient_color;
        Vector4 light_pos;
        Color light_color; // light type in a
        Vector4 light_atten;
        Vector4 spot_light_dir;
        Vector4 shadow_params; // strength, z_bias, slope_bias, filter_radius
    };

    struct MaterialProperty
    {
        static constexpr const char* TEXTURE = "u_texture";
        static constexpr const char* TEXTURE_SCALE_OFFSET = "u_texture_scale_offset";
        static constexpr const char* COLOR = "u_color";

        enum class Type
        {
            Color,
            Vector,
            Float,
            Range,
            Texture,
            Matrix,
            VectorArray,
            MatrixArray,
            Int,
        };

        union Data
        {
            float matrix[16];
            float vector[4];
            float color[4];
            float float_value;
            int int_value;
        };

        std::string name;
        Type type;
        Data data;
        std::shared_ptr<Texture> texture;
        std::vector<Vector4> vector_array;
        std::vector<Matrix4x4> matrix_array;
        int size = 0;
        bool dirty;
    };

    struct UniformBuffer
    {
        filament::backend::UniformBufferHandle uniform_buffer;
        ByteBuffer buffer;
        bool dirty = false;
    };

    struct Sampler
    {
        int binding;
        std::shared_ptr<Texture> texture;
    };

    struct SamplerGroup
    {
        filament::backend::SamplerGroupHandle sampler_group;
        std::vector<Sampler> samplers;
        bool dirty = false;
    };

    struct ShaderVariant
    {
        std::string key;
        std::vector<std::string> keywords;
        std::shared_ptr<Shader> shader;
    };

    // per renderer bones uniforms, set by skinned mesh renderer
    struct SkinnedMeshRendererUniforms
    {
        static constexpr const char* BONES = "u_bones";
        static constexpr const int BONES_VECTOR_MAX_COUNT = 210;

        Vector4 bones[BONES_VECTOR_MAX_COUNT];
    };

    typedef std::map<std::string, MaterialProperty> mProperty;
    typedef std::map<std::string, MaterialProperty>::iterator itProperty;
    typedef std::map<std::string, MaterialProperty>::const_iterator const_itProperty;

    class Material :
        public Object
    {
    public:
        Material();
        Material(const std::shared_ptr<Shader>& shader);
        ~Material();
        static void Done();
        static const std::shared_ptr<Material>& GetSharedBoundsMaterial();
        const std::string& GetShaderName();
		const Rect& GetScissorRect() const { return m_scissor_rect; }
		void SetScissorRect(const Rect& rect);
        std::string EnableKeywords(const std::vector<std::string>& keywords);
        const std::shared_ptr<Shader>& GetShader();
        const std::shared_ptr<Shader>& GetShader(const std::string& key);
        const std::shared_ptr<Shader>& GetShader(const std::vector<std::string>& keywords);
        int GetQueue() const;
        void SetQueue(int queue);
        const Matrix4x4* GetMatrix(const std::string& name) const;
        void SetMatrix(const std::string& name, const Matrix4x4& value);
        const Vector4* GetVector(const std::string& name) const;
        void SetVector(const std::string& name, const Vector4& value);
        void SetColor(const std::string& name, const Color& value);
        void SetFloat(const std::string& name, float value);
        void SetInt(const std::string& name, int value);
        std::shared_ptr<Texture> GetTexture(const std::string& name) const;
        void SetTexture(const std::string& name, const std::shared_ptr<Texture>& texture);
        void SetVectorArray(const std::string& name, const std::vector<Vector4>& array);
        void SetMatrixArray(const std::string& name, const std::vector<Matrix4x4>& array);
        template <class T>
        const T* GetProperty(const std::string& name, MaterialProperty::Type type) const
        {
            const MaterialProperty* property_ptr;
            const_itProperty it = m_properties.find(name);
            if (it != m_properties.end())
            {
                property_ptr = &it->second;
                if (property_ptr->type == type)
                {
                    return (const T*)&property_ptr->data;
                }
            }

            return nullptr;
        }
        template <class T>
        void SetProperty(const std::string &name, const T& v, MaterialProperty::Type type)
        {
            MaterialProperty* property_ptr;
            itProperty it = m_properties.find(name);
            if (it != m_properties.end())
            {
                property_ptr = &it->second;
                property_ptr->type = type;
                Memory::Copy(&property_ptr->data, &v, sizeof(v));
                property_ptr->dirty = true;
            }
            else
            {
                MaterialProperty property;
                property.name = name;
                property.type = type;
                Memory::Copy(&property.data, &v, sizeof(v));
                property.size = sizeof(v);
                property.dirty = true;
                m_properties[name] = property;
            }
        }
        void Prepare(int pass = -1);
        void SetScissor(int target_width, int target_height);

        void Bind(const std::shared_ptr<Shader>& shader, int pass);
        //pbr mat
        //////////////////////////////////////////////////////////////////////////
        enum PBR_WORKFLOW
        {
            PBR_WORKFLOW_METALL_ROUGH = 0,
            PBR_WORKFLOW_SPEC_GLOSS
        };

        enum ALPHA_MODE
        {
            ALPHA_MODE_OPAQUE = 0,
            ALPHA_MODE_MASK,
            ALPHA_MODE_BLEND,
            ALPHA_MODE_NUM_MODES
        };

        // Material attributes packed in a shader-friendly format
        struct ShaderAttribs
        {
            Vector4 BaseColorFactor = Vector4{ 1, 1, 1, 1 };
            Vector4 EmissiveFactor = Vector4{ 1, 1, 1, 1 };
            Vector4 SpecularFactor = Vector4{ 1, 1, 1, 1 };

            int   Workflow = PBR_WORKFLOW_METALL_ROUGH;
            float BaseColorUVSelector = -1;
            float PhysicalDescriptorUVSelector = -1;
            float NormalUVSelector = -1;

            float OcclusionUVSelector = -1;
            float EmissiveUVSelector = -1;
            float BaseColorSlice = 0;
            float PhysicalDescriptorSlice = 0;

            float NormalSlice = 0;
            float OcclusionSlice = 0;
            float EmissiveSlice = 0;
            float MetallicFactor = 1;

            float RoughnessFactor = 1;
            int   AlphaMode = ALPHA_MODE_OPAQUE;
            float AlphaCutoff = 0.5f;
            float Dummy0;

            // When texture atlas is used, UV scale and bias applied to
            // each texture coordinate set
            Vector4 BaseColorUVScaleBias = Vector4{ 1, 1, 0, 0 };
            Vector4 PhysicalDescriptorUVScaleBias = Vector4{ 1, 1, 0, 0 };
            Vector4 NormalUVScaleBias = Vector4{ 1, 1, 0, 0 };
            Vector4 OcclusionUVScaleBias = Vector4{ 1, 1, 0, 0 };
            Vector4 EmissiveUVScaleBias = Vector4{ 1, 1, 0, 0 };
        };
        static_assert(sizeof(ShaderAttribs) % 16 == 0, "ShaderAttribs struct must be 16-byte aligned");
        ShaderAttribs Attribs;

        bool DoubleSided = false;

        enum TEXTURE_ID
        {
            // Base color for metallic-roughness workflow or
            // diffuse color for specular-glossinees workflow
            TEXTURE_ID_BASE_COLOR = 0,

            // Metallic-roughness or specular-glossinees map
            TEXTURE_ID_PHYSICAL_DESC,

            TEXTURE_ID_NORMAL_MAP,
            TEXTURE_ID_OCCLUSION,
            TEXTURE_ID_EMISSIVE,
            TEXTURE_ID_NUM_TEXTURES
        };
        // Texture indices in Model.Textures array
        std::array<int, TEXTURE_ID_NUM_TEXTURES> TextureIds = {};
    private:
        void UpdateUniformMember(const std::string& name, const void* data, int size);
        void UpdateUniformTexture(const std::string& name, const std::shared_ptr<Texture>& texture);
        void UpdatePbrUniform();

        static std::shared_ptr<Material> m_shared_bounds_material;
        std::map<std::string, ShaderVariant> m_shader_variants;
        std::shared_ptr<int> m_queue;
        mProperty m_properties;
        Rect m_scissor_rect;
        std::vector<std::vector<UniformBuffer >> m_unifrom_buffers;
        std::vector<std::vector<SamplerGroup>> m_samplers;
        //pbr material
        filament::backend::UniformBufferHandle m_pbr_uniform_buffer;
        bool m_isPbr = false;
    };
}



