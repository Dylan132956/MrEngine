#include "Shader.h"
#include <string>
#include <vector>
#include "io/FileSystem.h"
#include "Engine.h"
#include <list>
#include "io/File.h"
#include "Debug.h"
#include "shader_converter.h"

namespace moonriver
{
    std::map<std::string, std::shared_ptr<Shader>> Shader::m_shaders;

    Shader::Shader(const std::string& name) :
        m_queue(0)
    {
        this->SetName(name);

        //std::string vs_path[1];
        //std::string fs_path[1];

        //std::string asset_path = Engine::Instance()->GetDataPath();

        //vs_path[0] = asset_path + "/shader/HLSL/simple.vert.hlsl";
        //fs_path[0] = asset_path + "/shader/HLSL/simple.frag.hlsl";
        //char* vs_buffer = FileSystem::ReadFileData(vs_path[0].c_str());
        //char* fs_buffer = FileSystem::ReadFileData(fs_path[0].c_str());

        //std::string vs_hlsl = vs_buffer;
        //std::string fs_hlsl = fs_buffer;
        //FileSystem::FreeFileData(vs_buffer);
        //FileSystem::FreeFileData(fs_buffer);

        //const char* c_vs_hlsl[1];
        //const char* c_fs_hlsl[1];
        //c_vs_hlsl[0] = vs_hlsl.c_str();
        //c_fs_hlsl[0] = fs_hlsl.c_str();

        //const char* c_vs_path[1];
        //const char* c_fs_path[1];
        //c_vs_path[0] = vs_path[0].c_str();
        //c_fs_path[0] = fs_path[0].c_str();

        //std::vector<unsigned int> vs_spriv;
        //int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
        //std::string entryPointName = "simple_vert_main";
        //CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

        //std::vector<unsigned int> fs_spriv;
        //option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
        //entryPointName = "simple_frag_main";
        //CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

        //std::string vs_glsl = compile_iteration(vs_spriv);

        //std::string fs_glsl = compile_iteration(fs_spriv);

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
        auto& driver = Engine::Instance()->GetDriverApi();

        for (int i = 0; i < m_passes.size(); ++i)
        {
            auto& pass = m_passes[i];

            if (pass.pipeline.program)
            {
                driver.destroyProgram(pass.pipeline.program);
                pass.pipeline.program.clear();
            }
        }
        m_passes.clear();
    }

    void Shader::Done()
    {
        m_shaders.clear();
    }

    static std::list<std::string> KeywordsToList(const std::vector<std::string>& keywords)
    {
        std::list<std::string> keyword_list;
        for (int i = 0; i < keywords.size(); ++i)
        {
            keyword_list.push_back(keywords[i]);
        }
        keyword_list.sort();
        return keyword_list;
    }

    std::string Shader::MakeKey(const std::string& name, const std::vector<std::string>& keywords)
    {
        std::string key = name;
        std::list<std::string> keyword_list = KeywordsToList(keywords);
        for (const auto& i : keyword_list)
        {
            key += "|" + i;
        }
        return key;
    }

    void Shader::Compile()
    {
        std::string define;
        if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL) {
        }
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
        {
            define = "#define COMPILER_HLSL 1\n";
        }
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::VULKAN)
        {
            define = "#define COMPILER_VULKAN 1\n";
        }

        for (const auto& i : m_keywords)
        {
            define += "#define " + i + " 1\n";
        }

        for (int i = 0; i < m_passes.size(); ++i)
        {
            auto& pass = m_passes[i];
            std::string vs_hlsl = pass.vs;
            std::string fs_hlsl = pass.fs;
            std::vector<char> vs_data;
            std::vector<char> fs_data;
            if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL) {
                std::string vs_path[1];
                std::string fs_path[1];
                vs_path[0] = pass.vsPath;
                fs_path[0] = pass.fsPath;
                const char* c_vs_hlsl[1];
                const char* c_fs_hlsl[1];
                vs_hlsl = define + vs_hlsl;
                fs_hlsl = define + fs_hlsl;
                c_vs_hlsl[0] = vs_hlsl.c_str();
                c_fs_hlsl[0] = fs_hlsl.c_str();

                const char* c_vs_path[1];
                const char* c_fs_path[1];
                c_vs_path[0] = vs_path[0].c_str();
                c_fs_path[0] = fs_path[0].c_str();

                std::vector<unsigned int> vs_spriv;
                int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                std::string entryPointName = "vert";
                CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

                std::vector<unsigned int> fs_spriv;
                option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                entryPointName = "frag";
                CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

                compile_arguments arg;

                arg.version = 330;
                arg.set_version = true;
#if VR_ANDROID || VR_IOS
                arg.set_es = true;
                arg.es = true;
                arg.version = 300;
                arg.set_version = true;
#endif
                std::string vs_glsl = spirv_converter(arg, vs_spriv);
                std::string fs_glsl = spirv_converter(arg, fs_spriv);

                vs_data.resize(vs_glsl.size());
                memcpy(&vs_data[0], &vs_glsl[0], vs_data.size());
                fs_data.resize(fs_glsl.size());
                memcpy(&fs_data[0], &fs_glsl[0], fs_data.size());
            }
            else if (Engine::Instance()->GetBackend() == filament::backend::Backend::METAL)
            {
                std::string vs_path[1];
                std::string fs_path[1];
                vs_path[0] = pass.vsPath;
                fs_path[0] = pass.fsPath;
                const char* c_vs_hlsl[1];
                const char* c_fs_hlsl[1];
                vs_hlsl = define + vs_hlsl;
                fs_hlsl = define + fs_hlsl;
                c_vs_hlsl[0] = vs_hlsl.c_str();
                c_fs_hlsl[0] = fs_hlsl.c_str();

                const char* c_vs_path[1];
                const char* c_fs_path[1];
                c_vs_path[0] = vs_path[0].c_str();
                c_fs_path[0] = fs_path[0].c_str();

                std::vector<unsigned int> vs_spriv;
                int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                std::string entryPointName = "vert";
                CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

                std::vector<unsigned int> fs_spriv;
                option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                entryPointName = "frag";
                CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

                compile_arguments arg;

#if VR_MAC || VR_IOS
                arg.msl = true;
                arg.msl_decoration_binding = true;
#endif
#if VR_IOS
                arg.msl_ios = true;
#endif
                std::string vs_msl = spirv_converter(arg, vs_spriv);
                std::string fs_msl = spirv_converter(arg, fs_spriv);

                vs_data.resize(vs_msl.size());
                memcpy(&vs_data[0], &vs_msl[0], vs_data.size());
                fs_data.resize(fs_msl.size());
                memcpy(&fs_data[0], &fs_msl[0], fs_data.size());
            }
            else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
            {
                std::string vs = define + vs_hlsl;
                std::string fs = define + fs_hlsl;
                vs_data.resize(vs.size());
                memcpy(&vs_data[0], &vs[0], vs_data.size());
                fs_data.resize(fs.size());
                memcpy(&fs_data[0], &fs[0], fs_data.size());
            }
            else if (Engine::Instance()->GetBackend() == filament::backend::Backend::VULKAN)
            {
                const char* c_vs_hlsl[1];
                const char* c_fs_hlsl[1];

                std::string vs_path[1];
                std::string fs_path[1];
                vs_path[0] = pass.vsPath;
                fs_path[0] = pass.fsPath;

                vs_hlsl = define + vs_hlsl;
                fs_hlsl = define + fs_hlsl;

                c_vs_hlsl[0] = vs_hlsl.c_str();
                c_fs_hlsl[0] = fs_hlsl.c_str();

                const char* c_vs_path[1];
                const char* c_fs_path[1];
                c_vs_path[0] = vs_path[0].c_str();
                c_fs_path[0] = fs_path[0].c_str();

                std::vector<unsigned int> vs_spriv;
                int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                std::string entryPointName = "vert";
                CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

                std::vector<unsigned int> fs_spriv;
                option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
                entryPointName = "frag";
                CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

                vs_data.resize(vs_spriv.size() * 4);
                memcpy(&vs_data[0], &vs_spriv[0], vs_data.size());
                fs_data.resize(fs_spriv.size() * 4);
                memcpy(&fs_data[0], &fs_spriv[0], fs_data.size());
            }

            SetUniform(pass, GetName(), vs_data, fs_data);
        }
    }

    void Shader::SetUniform(Pass& pass, const std::string& shaderName, std::vector<char>& vs_data, std::vector<char>& fs_data)
    {
        if (shaderName == "standard") {
            {
                Uniform u;
                u.name = "PerView";
                u.binding = 0;
                pass.uniforms.push_back(u);
            }
            {
                Uniform u;
				u.name = "PerRenderer";
				u.binding = 1;
				pass.uniforms.push_back(u);
            }
            {
                Uniform u;
				u.name = "PerMaterialVertex";
				u.binding = 2;
				pass.uniforms.push_back(u);
            }
            {
                Uniform u;
				u.name = "PerMaterialFragment";
				u.binding = 4;
				pass.uniforms.push_back(u);
            }
            SamplerGroup group;
            group.name = "PerMaterialFragment";
            group.binding = 4;

            Sampler sampler;
            sampler.name = "SPIRV_Cross_Combinedg_ColorMapg_ColorSampler";
            sampler.binding = 0;
            group.samplers.push_back(sampler);

            sampler.name = "SPIRV_Cross_Combinedg_PhysicalDescriptorMapg_PhysicalDescriptorSampler";
            sampler.binding = 1;
            group.samplers.push_back(sampler);

            sampler.name = "SPIRV_Cross_Combinedg_NormalMapg_NormalSampler";
            sampler.binding = 2;
            group.samplers.push_back(sampler);

            sampler.name = "SPIRV_Cross_Combinedg_AOMapg_AOSampler";
            sampler.binding = 3;
            group.samplers.push_back(sampler);

            sampler.name = "SPIRV_Cross_Combinedg_EmissiveMapg_EmissiveSampler";
            sampler.binding = 4;
            group.samplers.push_back(sampler);

            pass.samplers.push_back(group);

            //material: TODO
            pass.pipeline.rasterState.depthWrite = true;
            pass.pipeline.rasterState.colorWrite = true;

            pass.pipeline.rasterState.culling = filament::backend::RasterState::CullingMode::BACK;
        }
        else if (shaderName == "ui")
        {
            {
                Uniform u;
                u.name = "PerView";
                u.binding = 0;
                pass.uniforms.push_back(u);
            }
            {
                Uniform u;
				u.name = "PerRenderer";
				u.binding = 1;
				pass.uniforms.push_back(u);
            }
            {

                Uniform u;
                u.name = "PerMaterialVertex";
                u.binding = 3;
                Member member;
                member.name = "u_texture_scale_offset";
                member.size = 16;
                member.offset = 0;
                u.members.push_back(member);
                u.size = 16;
                pass.uniforms.push_back(u);
            }
            {
				Uniform u;
                Member member;
				u.name = "PerMaterialFragment";
				u.binding = 4;
				member.name = "u_color";
				member.size = 16;
				member.offset = 0;
				u.members.push_back(member);
				u.size = 16;
				pass.uniforms.push_back(u);
            }
			SamplerGroup group;
			group.name = "PerMaterialFragment";
			group.binding = 4;
			Sampler sampler;
			sampler.name = "SPIRV_Cross_Combinedg_ColorMapg_ColorSampler";
			sampler.binding = 0;
			group.samplers.push_back(sampler);
			pass.samplers.push_back(group);

			//material: TODO
			pass.pipeline.rasterState.depthWrite = false;
			pass.pipeline.rasterState.colorWrite = true;
            //blend
			pass.pipeline.rasterState.blendFunctionSrcRGB = filament::backend::BlendFunction::SRC_ALPHA;
			pass.pipeline.rasterState.blendFunctionDstRGB = filament::backend::BlendFunction::ONE_MINUS_SRC_ALPHA;
			pass.pipeline.rasterState.blendFunctionSrcAlpha = filament::backend::BlendFunction::SRC_ALPHA;
			pass.pipeline.rasterState.blendFunctionDstAlpha = filament::backend::BlendFunction::ONE_MINUS_SRC_ALPHA;
			//disable depthtest
            pass.pipeline.rasterState.depthFunc = filament::backend::RasterState::DepthFunc::A;
            pass.pipeline.rasterState.culling = filament::backend::RasterState::CullingMode::NONE;
        }

		auto& driver = Engine::Instance()->GetDriverApi();

		filament::backend::Program pb;
		pb.diagnostics(utils::CString(shaderName.c_str()))
			.withVertexShader((void*)&vs_data[0], vs_data.size())
			.withFragmentShader((void*)&fs_data[0], fs_data.size());

		for (int i = 0; i < pass.uniforms.size(); ++i)
		{
			pb.setUniformBlock(pass.uniforms[i].binding, utils::CString(pass.uniforms[i].name.c_str()));
		}

		for (int i = 0; i < pass.samplers.size(); ++i)
		{
			const auto& group = pass.samplers[i];

			std::vector<filament::backend::Program::Sampler> samplers;
			for (int j = 0; j < group.samplers.size(); ++j)
			{
				filament::backend::Program::Sampler sampler;
				sampler.name = utils::CString(group.samplers[j].name.c_str());
				sampler.binding = group.samplers[j].binding;
				samplers.push_back(sampler);
			}
			pb.setSamplerGroup((size_t)group.binding, &samplers[0], samplers.size());
		}

		pass.pipeline.program = driver.createProgram(std::move(pb));
    }

    std::shared_ptr<Shader>Shader::Find(const std::string& name, const std::vector<std::string>& keywords)
    {
        std::shared_ptr<Shader> shader;
        std::string key = MakeKey(name, keywords);
        std::map<std::string, std::shared_ptr<Shader>>::iterator it = m_shaders.find(key);
        if (it != m_shaders.end()) {
            shader = it->second;
        }
        else
        {
            shader = std::make_shared<Shader>(name);
            std::string vs_path[1];
            std::string fs_path[1];

            std::string asset_path = Engine::Instance()->GetDataPath();

            vs_path[0] = asset_path + "/shader/HLSL/" + name + ".vert.hlsl";
            fs_path[0] = asset_path + "/shader/HLSL/" + name + ".frag.hlsl";

            if (File::Exist(vs_path[0]) && File::Exist(fs_path[0]))
            {
                //only one pass current
                shader->m_passes.resize(1);
                char* vs_buffer = FileSystem::ReadFileData(vs_path[0].c_str());
                char* fs_buffer = FileSystem::ReadFileData(fs_path[0].c_str());
                shader->m_passes[0].vs = vs_buffer;
                shader->m_passes[0].fs = fs_buffer;
                FileSystem::FreeFileData(vs_buffer);
                FileSystem::FreeFileData(fs_buffer);
                shader->m_passes[0].vsPath = vs_path[0];
                shader->m_passes[0].fsPath = fs_path[0];
                shader->m_shader_key = key;
                shader->m_keywords = keywords;
                shader->Compile();
                m_shaders[key] = shader;
            }
        }

        return shader;
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
