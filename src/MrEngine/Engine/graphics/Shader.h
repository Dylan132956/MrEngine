#pragma once
#include "math/Matrix4x4.h"
#include "Object.h"
#include "private/backend/DriverApi.h"
#include <map>

namespace moonriver
{
    struct mvpUniforms
    {
        static constexpr const char* M_MATRIX = "u_WorldMatrix";
        static constexpr const char* V_MATRIX = "u_ViewMatrix";
        static constexpr const char* P_MATRIX = "u_ProjectionMatrix";
        Matrix4x4 uWorldMatrix;
        Matrix4x4 uViewMatrix;
        Matrix4x4 uProjectionMatrix;
    };

    class Shader : public Object
    {
    public:
        enum class AttributeLocation_Cube
        {
            Vertex = 0,
            Color = 1,
            UV = 2,

            Count = 3
        };

        enum class BindingPoint
        {
            PerView = 0,
            PerRenderer = 1,
            PerRendererBones = 2,
            PerMaterialVertex = 3,
            PerMaterialFragment = 4,
            PerLightVertex = 5,
            PerLightFragment = 6,

            Count = filament::backend::CONFIG_UNIFORM_BINDING_COUNT,
        };

        enum class AttributeLocation
        {
            Vertex = 0,
            Color = 1,
            UV = 2,
            UV2 = 3,
            Normal = 4,
            Tangent = 5,
            BoneWeights = 6,
            BoneIndices = 7,

            Count = filament::backend::MAX_VERTEX_ATTRIBUTE_COUNT
        };

        enum class Queue
        {
            Background = 1000,
            Geometry = 2000,
            AlphaTest = 2450,
            Transparent = 3000,
            Overlay = 4000,
        };

        enum class LightMode
        {
            None = 0,
            Forward = 1,
        };

        struct Member
        {
            std::string name;
            int offset;
            int size;
        };

        struct Uniform
        {
            std::string name;
            int binding;
            std::vector<Member> members;
            int size;
        };

        struct Sampler
        {
            std::string name;
            int binding;
        };

        struct SamplerGroup
        {
            std::string name;
            int binding;
            std::vector<Sampler> samplers;
        };

        struct Pass
        {
            std::string vs;
            std::string fs;
            std::string vsPath;
            std::string fsPath;
            int queue = (int)Queue::Geometry;
            LightMode light_mode = LightMode::None;
            std::vector<Uniform> uniforms;
            std::vector<SamplerGroup> samplers;
            filament::backend::PipelineState pipeline;
        };

        Shader(const std::string& name);
        ~Shader();
        const std::string& GetShaderKey() const { return m_shader_key; }
        const std::vector<std::string>& GetKeywords() const { return m_keywords; }
        static std::string MakeKey(const std::string& name, const std::vector<std::string>& keywords);
        static std::shared_ptr<Shader> Find(const std::string& name, const std::vector<std::string>& keywords = std::vector<std::string>());
        int GetPassCount() const { return m_passes.size(); }
        const Pass& GetPass(int index) const { return m_passes[index]; }
        int GetQueue() const { return m_queue; }
        void Compile();
        //TODO: parse material renderstate
        void SetRenderState(Pass& pass, const std::string& shaderName, std::vector<char>& vs, std::vector<char>& fs);

        static void Init();
        static void Done();
        static void Exit();
    private:
        static std::map<std::string, std::shared_ptr<Shader>> m_shaders;
        std::string m_shader_key;
        std::vector<std::string> m_keywords;
        std::vector<Pass> m_passes;
        int m_queue;
    };
}
