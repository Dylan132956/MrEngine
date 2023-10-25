#include "Shader.h"
#include <string>
#include <vector>
#include "io/FileSystem.h"
#include "Engine.h"

namespace moonriver
{
    Shader::Shader()
    {
        std::string vs_path[1];
        std::string fs_path[1];

        std::string asset_path = Engine::Instance()->GetDataPath();

        vs_path[0] = asset_path + "/shader/HLSL/simple.vert.hlsl";
        fs_path[0] = asset_path + "/shader/HLSL/simple.frag.hlsl";
        char* vs_buffer = FileSystem::ReadFileData(vs_path[0].c_str());
        char* fs_buffer = FileSystem::ReadFileData(fs_path[0].c_str());

        std::string vs_hlsl = vs_buffer;
        std::string fs_hlsl = fs_buffer;
        FileSystem::FreeFileData(vs_buffer);
        FileSystem::FreeFileData(fs_buffer);

        const char* c_vs_hlsl[1];
        const char* c_fs_hlsl[1];
        c_vs_hlsl[0] = vs_hlsl.c_str();
        c_fs_hlsl[0] = fs_hlsl.c_str();

        const char* c_vs_path[1];
        const char* c_fs_path[1];
        c_vs_path[0] = vs_path[0].c_str();
        c_fs_path[0] = fs_path[0].c_str();

        std::vector<unsigned int> vs_spriv;
        int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
        std::string entryPointName = "simple_vert_main";
        CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

        std::vector<unsigned int> fs_spriv;
        option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
        entryPointName = "simple_frag_main";
        CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

        std::string vs_glsl = compile_iteration(vs_spriv);

        std::string fs_glsl = compile_iteration(fs_spriv);

        //spirv_cross::CompilerGLSL vs_compiler(&vs_spriv[0], vs_spriv.size());
        //std::string vs_glsl = vs_compiler.compile();

        //spirv_cross::CompilerGLSL fs_compiler(&fs_spriv[0], fs_spriv.size());
        //spirv_cross::CompilerGLSL::Options opts = fs_compiler.get_common_options();
        //opts.version = 420;
        //opts.enable_420pack_extension = true;
        //opts.force_recompile_max_debug_iterations = 3;
        //opts.enable_storage_image_qualifier_deduction = true;
        //opts.enable_row_major_load_workaround = true;
        //fs_compiler.set_common_options(opts);


        //std::string fs_glsl = fs_compiler.compile();
    }

    Shader::~Shader()
    {

    }

    void Shader::Exit()
    {
        glslang::FinalizeProcess();
    }

    void Shader::Init()
    {
        glslang::InitializeProcess();
    }
}