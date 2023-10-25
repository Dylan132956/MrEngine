#pragma once

#include "glslang/Public/ShaderLang.h"
#include "spirv_glsl.hpp"
#include "math/Matrix4x4.h"
#include "Object.h"

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
        Shader();
        ~Shader();
        static void Init();
        static void Exit();
    };
}

void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);
std::string compile_iteration(std::vector<uint32_t>& spirv_file);