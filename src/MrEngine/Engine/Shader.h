#include "glslang/Public/ShaderLang.h"
#include "spirv_glsl.hpp"

namespace moonriver
{
    class Shader
    {
    public:
        Shader();
        ~Shader();
        static void Init();
        static void Exit();
    };
}

void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);
std::string compile_iteration(std::vector<uint32_t>& spirv_file);