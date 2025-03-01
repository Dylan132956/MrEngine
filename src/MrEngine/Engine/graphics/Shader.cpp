#include "Shader.h"
#include <string>
#include <vector>
#include "io/FileSystem.h"
#include "Engine.h"
#include <list>
#include "io/File.h"
#include "Debug.h"
#include "shader_converter.h"
#include "graphics/sprivShader/spirv_shader.h"
#include "string/Utils.h"
#include "lua/lua.hpp"
#include "Debug.h"

namespace moonriver
{
    std::map<std::string, std::shared_ptr<Shader>> Shader::m_shaders;

	static void SetGlobalInt(lua_State* L, const char* key, int value)
	{
		lua_pushinteger(L, value);
		lua_setglobal(L, key);
	}

	static void GetTableString(lua_State* L, const char* key, std::string& str)
	{
		lua_pushstring(L, key);
		lua_gettable(L, -2);
		if (lua_isstring(L, -1))
		{
			str = lua_tostring(L, -1);
		}
		lua_pop(L, 1);
	}

	template <class T>
	static void GetTableInt(lua_State* L, const char* key, T& num)
	{
		lua_pushstring(L, key);
		lua_gettable(L, -2);
		if (lua_isinteger(L, -1))
		{
			num = (T)lua_tointeger(L, -1);
		}
		lua_pop(L, 1);
	}

    Shader::Shader(const std::string& name) :
        m_queue(0)
    {
        this->SetName(name);
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
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11 || Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
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
            //spriv shader compile and shader reflection
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
			int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17) | (1 << 8);
			std::string entryPointName = "vert";
			converter_spirv(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv, &pass);

			std::vector<unsigned int> fs_spriv;
			option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17) | (1 << 8);
			entryPointName = "frag";
			converter_spirv(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv, &pass);

            if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL) {
                compile_arguments arg;
#if VR_ANDROID || VR_IOS
				arg.set_version = true;
				arg.es = true;
				arg.set_es = true;
				if (Engine::Instance()->GetShaderModel() == filament::backend::ShaderModel::GL_ES_20)
				{
					arg.version = 100;
				}
				else
				{
					arg.version = 300;
				}
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
            else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11 || Engine::Instance()->GetBackend() == filament::backend::Backend::D3D12)
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
                vs_data.resize(vs_spriv.size() * 4);
                memcpy(&vs_data[0], &vs_spriv[0], vs_data.size());
                fs_data.resize(fs_spriv.size() * 4);
                memcpy(&fs_data[0], &fs_spriv[0], fs_data.size());
            }

			Apply(pass, GetName(), vs_data, fs_data);
        }
    }

	void Shader::SetRenderState(Pass& pass, const std::string& str_param)
	{
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		SetGlobalInt(L, "Off", 0);
		SetGlobalInt(L, "On", 1);

		SetGlobalInt(L, "Back", (int)filament::backend::CullingMode::BACK);
		SetGlobalInt(L, "Front", (int)filament::backend::CullingMode::FRONT);

		SetGlobalInt(L, "Less", (int)filament::backend::SamplerCompareFunc::L);
		SetGlobalInt(L, "Greater", (int)filament::backend::SamplerCompareFunc::G);
		SetGlobalInt(L, "LEqual", (int)filament::backend::SamplerCompareFunc::LE);
		SetGlobalInt(L, "GEqual", (int)filament::backend::SamplerCompareFunc::GE);
		SetGlobalInt(L, "Equal", (int)filament::backend::SamplerCompareFunc::E);
		SetGlobalInt(L, "NotEqual", (int)filament::backend::SamplerCompareFunc::NE);
		SetGlobalInt(L, "Always", (int)filament::backend::SamplerCompareFunc::A);

		SetGlobalInt(L, "Zero", (int)filament::backend::BlendFunction::ZERO);
		SetGlobalInt(L, "One", (int)filament::backend::BlendFunction::ONE);
		SetGlobalInt(L, "SrcColor", (int)filament::backend::BlendFunction::SRC_COLOR);
		SetGlobalInt(L, "SrcAlpha", (int)filament::backend::BlendFunction::SRC_ALPHA);
		SetGlobalInt(L, "DstColor", (int)filament::backend::BlendFunction::DST_COLOR);
		SetGlobalInt(L, "DstAlpha", (int)filament::backend::BlendFunction::DST_ALPHA);
		SetGlobalInt(L, "OneMinusSrcColor", (int)filament::backend::BlendFunction::ONE_MINUS_SRC_COLOR);
		SetGlobalInt(L, "OneMinusSrcAlpha", (int)filament::backend::BlendFunction::ONE_MINUS_SRC_ALPHA);
		SetGlobalInt(L, "OneMinusDstColor", (int)filament::backend::BlendFunction::ONE_MINUS_DST_COLOR);
		SetGlobalInt(L, "OneMinusDstAlpha", (int)filament::backend::BlendFunction::ONE_MINUS_DST_ALPHA);

		SetGlobalInt(L, "Background", (int)Queue::Background);
		SetGlobalInt(L, "Geometry", (int)Queue::Geometry);
		SetGlobalInt(L, "AlphaTest", (int)Queue::AlphaTest);
		SetGlobalInt(L, "Transparent", (int)Queue::Transparent);
		SetGlobalInt(L, "Overlay", (int)Queue::Overlay);

		if (luaL_dostring(L, str_param.c_str()) != 0)
		{
			std::string error = lua_tostring(L, -1);
			lua_pop(L, 1);
			Log("do lua error: %s\n", error.c_str());
			assert(false);
		}
		lua_getglobal(L, "rs");
		// rs
		filament::backend::CullingMode culling = filament::backend::CullingMode::BACK;
		GetTableInt(L, "Cull", culling);
		pass.pipeline.rasterState.culling = culling;

		filament::backend::SamplerCompareFunc depth_func = filament::backend::SamplerCompareFunc::LE;
		GetTableInt(L, "ZTest", depth_func);
		pass.pipeline.rasterState.depthFunc = depth_func;

		bool depth_write = true;
		GetTableInt(L, "ZWrite", depth_write);
		pass.pipeline.rasterState.depthWrite = depth_write;

		filament::backend::BlendFunction src_color_blend = filament::backend::BlendFunction::ONE;
		filament::backend::BlendFunction dst_color_blend = filament::backend::BlendFunction::ZERO;
		filament::backend::BlendFunction src_alpha_blend = filament::backend::BlendFunction::ONE;
		filament::backend::BlendFunction dst_alpha_blend = filament::backend::BlendFunction::ZERO;
		GetTableInt(L, "SrcBlendMode", src_color_blend);
		GetTableInt(L, "DstBlendMode", dst_color_blend);
		GetTableInt(L, "SrcBlendMode", src_alpha_blend);
		GetTableInt(L, "DstBlendMode", dst_alpha_blend);
		GetTableInt(L, "SrcColorBlendMode", src_color_blend);
		GetTableInt(L, "DstColorBlendMode", dst_color_blend);
		GetTableInt(L, "SrcAlphaBlendMode", src_alpha_blend);
		GetTableInt(L, "DstAlphaBlendMode", dst_alpha_blend);
		pass.pipeline.rasterState.blendFunctionSrcRGB = src_color_blend;
		pass.pipeline.rasterState.blendFunctionSrcAlpha = src_alpha_blend;
		pass.pipeline.rasterState.blendFunctionDstRGB = dst_color_blend;
		pass.pipeline.rasterState.blendFunctionDstAlpha = dst_alpha_blend;

		bool color_write = true;
		GetTableInt(L, "CWrite", color_write);
		pass.pipeline.rasterState.colorWrite = color_write;

		GetTableInt(L, "Queue", pass.queue);

		if (m_queue < pass.queue)
		{
			m_queue = pass.queue;
		}

		GetTableInt(L, "LightMode", pass.light_mode);

		std::vector<std::string>::iterator it = std::find(m_keywords.begin(), m_keywords.end(), "LIGHT_ADD_ON");
		if (pass.light_mode == LightMode::Forward && it != m_keywords.end())
		{
			pass.pipeline.rasterState.depthWrite = false;
			pass.pipeline.rasterState.blendFunctionSrcRGB = src_color_blend;
			pass.pipeline.rasterState.blendFunctionSrcAlpha = src_alpha_blend;
			pass.pipeline.rasterState.blendFunctionDstRGB = filament::backend::BlendFunction::ONE;
			pass.pipeline.rasterState.blendFunctionDstAlpha = filament::backend::BlendFunction::ONE;
		}
		lua_pop(L, 1);
		lua_close(L);
	}

    void Shader::Apply(Pass& pass, const std::string& shaderName, std::vector<char>& vs_data, std::vector<char>& fs_data)
    {
		auto& driver = Engine::Instance()->GetDriverApi();

        std::string asset_path = Engine::Instance()->GetDataPath();
        std::string shaderPath = asset_path + "/shader/HLSL/" + shaderName;
		filament::backend::Program pb;
		pb.diagnostics(utils::CString(shaderPath.c_str()))
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
            //std::string vs_path[1];
            //std::string fs_path[1];
			std::string shader_path[1];
			std::string asset_path = Engine::Instance()->GetDataPath();
            //vs_path[0] = asset_path + "/shader/HLSL/" + name + ".vert.hlsl";
            //fs_path[0] = asset_path + "/shader/HLSL/" + name + ".frag.hlsl";
			shader_path[0] = asset_path + "/shader/HLSL/" + name + ".shader";

            if (File::Exist(shader_path[0]))
            {
				char* shader_buffer = FileSystem::ReadFileData(shader_path[0].c_str());
				std::string str_shader = shader_buffer;
				FileSystem::FreeFileData(shader_buffer);
				RemoveUTF8BOM(str_shader);
				// ����pass
				size_t beginToken = std::string::npos;
				size_t endToken = std::string::npos;
				std::vector<std::string> totalPassNames;
				const std::string beginPass = "#DEF_PASSES";
				const std::string endPass = "#END_PASSES";
				bool bfind = FindInTokensString(str_shader, beginPass,
					endPass, 0, beginToken, endToken);

				while (bfind)
				{
					Pass pass;
					std::string passname = FindNextWord(str_shader, beginToken + beginPass.length() + 1);
					size_t passNamePos = FindTokenInText(str_shader, passname, beginToken + beginPass.length() + 1);
					size_t passNameEndPos = passNamePos + passname.length();
					std::string str_pass = str_shader.substr(passNameEndPos, endToken - passNameEndPos);
					const std::string beginParam = "#DEF_PARAMS";
					const std::string endParam = "#END_PARAMS";
					size_t beginTokenParam = std::string::npos;
					size_t endTokenParam = std::string::npos;
					bfind = FindInTokensString(str_pass, beginParam,
						endParam, 0, beginTokenParam, endTokenParam);
					std::string str_param = str_pass.substr(beginTokenParam + beginParam.length(), endTokenParam - beginTokenParam - beginParam.length());
					shader->SetRenderState(pass, str_param);

					const std::string cgprogram("CGPROGRAM");
					const std::string endcg("ENDCG");
					size_t beginTokenProgram = std::string::npos;
					size_t endTokenProgram = std::string::npos;
					bfind = FindInTokensString(str_pass, cgprogram,
						endcg, 0, beginTokenProgram, endTokenProgram);
					std::string str_program = str_pass.substr(beginTokenProgram + cgprogram.length(), endTokenProgram - beginTokenProgram - cgprogram.length());

					pass.vs = str_program;
					pass.fs = str_program;
					pass.vsPath = shader_path[0];
					pass.fsPath = shader_path[0];
					shader->m_passes.push_back(pass);
					//totalPassNames.push_back(passname);
					// �����µ�pass
					bfind = FindInTokensString(str_shader, beginPass,
						endPass, endToken, beginToken, endToken);
				}

				shader->m_shader_key = key;
				shader->m_keywords = keywords;
				m_shaders[key] = shader;
				shader->Compile();
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
