#include "triangle.h"
#include "Engine.h"
#include "Shader.h"
#include "FileSystem.h"
#include "glslang/Public/ShaderLang.h"
#include "spirv_glsl.hpp"

static const filament::math::float2 TRIANGLE_VERTICES[3] = { {1, 0}, {-0.0, 0.866}, {-0.5, -0.866} };
static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };


void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);

std::string compile_iteration(std::vector<uint32_t>& spirv_file);

namespace moonriver
{

    triangle::triangle()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        filament::backend::BufferUsage usage = filament::backend::BufferUsage::DYNAMIC;
        m_attributes[0].offset = 0;
        m_attributes[0].stride = sizeof(float) * 2;
        m_attributes[0].buffer = 0;
        m_attributes[0].type = filament::backend::ElementType::FLOAT2;
        m_vb = driver.createVertexBuffer(1, 1, 3, m_attributes, usage);
        driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(TRIANGLE_VERTICES, 24, nullptr), 0);
        filament::backend::ElementType index_type = filament::backend::ElementType::USHORT;
        m_ib = driver.createIndexBuffer(index_type, 3, usage);
        driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr), 0);
        m_primitives.resize(1);
        m_primitives[0] = driver.createRenderPrimitive();
        driver.setRenderPrimitiveBuffer(m_primitives[0], m_vb, m_ib, 1);
        driver.setRenderPrimitiveRange(m_primitives[0], filament::backend::PrimitiveType::TRIANGLES, 0, 0, 2, 3);

        //////////////////////////////////////////////////////////////////////////
        std::string vs_path[1];
        std::string fs_path[1];

        std::string asset_path = Engine::Instance()->GetDataPath();

        vs_path[0] = asset_path + "/shader/HLSL/triangle.vert.hlsl";
        fs_path[0] = asset_path + "/shader/HLSL/triangle.frag.hlsl";
        std::string vs_hlsl = FileSystem::ReadFileData(vs_path[0].c_str());
        std::string fs_hlsl = FileSystem::ReadFileData(fs_path[0].c_str());
        //////////////////////////////////////////////////////////////////////////
        std::string vs;
        std::string fs;
        std::string vfs;
        if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL) {
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
            std::string entryPointName = "vert";
            CompileAndLinkShader(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

            std::vector<unsigned int> fs_spriv;
            option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
            entryPointName = "frag";
            CompileAndLinkShader(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

            std::string vs_glsl = compile_iteration(vs_spriv);

            std::string fs_glsl = compile_iteration(fs_spriv);

            vs = vs_glsl;
            fs = fs_glsl;
            //vs = R"(
            //    attribute vec4 i_vertex;
            //    void main()
            //    {
	           //     gl_Position = vec4(i_vertex.xy, 0.0, 1.0);
            //    }
            //    )";
            //fs = R"(
            //    //#extension GL_ARB_gpu_shader5: enable
            //    void main()
            //    {
	           //     gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            //    }
            //    )";
        }
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
        {
            vs = vs_hlsl;
            fs = fs_hlsl;
            //vs = R"(
            //struct a2v_simple {
            //    float4 inputPosition : TEXCOORD0;
            //};
            //struct simple_vert_output
            //{
            //    float4 pos : SV_Position;
            //};
            //simple_vert_output vert(a2v_simple a)
            //{
            //    simple_vert_output o;
            //    o.pos = float4(a.inputPosition.x, a.inputPosition.y, 0.0, 1.0);
            //    return o;
            //}
            //)";
            //fs = R"(
            //struct simple_vert_output
            //{
            //    float4 pos : SV_Position;
            //};
            //struct SPIRV_Cross_Output
            //{
            //    float4 o_color : SV_Target0;
            //};
            //SPIRV_Cross_Output frag(simple_vert_output _entryPointOutput)
            //{
            //    SPIRV_Cross_Output stage_output;
            //    stage_output.o_color = float4(1.0, 0.0, 0.0, 1.0);
            //    return stage_output;
            //}
            //)";
            /////
            //vfs = R"(
            //    struct a2v_simple {
            //        float4 inputPosition : TEXCOORD0;
            //    };
            //    struct simple_vert_output
            //    {
            //        float4 pos : SV_Position;
            //    };
            //    simple_vert_output vert(a2v_simple a)
            //    {
            //        simple_vert_output o;
            //        o.pos = float4(a.inputPosition.x, a.inputPosition.y, 0.0, 1.0);
            //        return o;
            //    }
            //    struct SPIRV_Cross_Output
            //    {
            //        float4 o_color : SV_Target0;
            //    };
            //    SPIRV_Cross_Output frag(simple_vert_output _entryPointOutput)
            //    {
            //        SPIRV_Cross_Output stage_output;
            //        stage_output.o_color = float4(1.0, 0.0, 0.0, 1.0);
            //        return stage_output;
            //    }
            //)";
        }

        std::vector<char> vs_data;
        std::vector<char> fs_data;

        vs_data.resize(vs.size());
        memcpy(&vs_data[0], &vs[0], vs_data.size());
        fs_data.resize(fs.size());
        memcpy(&fs_data[0], &fs[0], fs_data.size());

        filament::backend::Program pb;
        pb.diagnostics(utils::CString("Assets/shader/HLSL/triangle"))
            .withVertexShader((void*)&vs_data[0], vs_data.size())
            .withFragmentShader((void*)&fs_data[0], fs_data.size());

        pipeline.program = driver.createProgram(std::move(pb));

        pipeline.rasterState.depthWrite = false;
        pipeline.rasterState.colorWrite = true;
        //disable depthtest
        pipeline.rasterState.depthFunc = filament::backend::RasterState::DepthFunc::A;
        pipeline.rasterState.culling = filament::backend::RasterState::CullingMode::NONE;

        Shader shader;
    }

    triangle::~triangle()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        driver.destroyVertexBuffer(m_vb);
        m_vb.clear();

        driver.destroyIndexBuffer(m_ib);
        m_ib.clear();

        if (pipeline.program) {
            driver.destroyProgram(pipeline.program);
            pipeline.program.clear();
        }

        for (int i = 0; i < m_primitives.size(); ++i)
        {
            driver.destroyRenderPrimitive(m_primitives[i]);
            m_primitives[i].clear();
        }
        m_primitives.clear();
    }

    void triangle::run()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        filament::backend::RenderTargetHandle target = *(filament::backend::RenderTargetHandle*)Engine::Instance()->GetDefaultRenderTarget();
        filament::backend::RenderPassParams params;
        params.flags.clear = filament::backend::TargetBufferFlags::COLOR;//| filament::backend::TargetBufferFlags::DEPTH;

        params.viewport.left = (int32_t)0;
        params.viewport.bottom = (int32_t)0;
        params.viewport.width = (uint32_t)1280;
        params.viewport.height = (uint32_t)720;
        params.clearColor = filament::math::float4(0.0, 1.0, 0.0, 1.0);

        driver.beginRenderPass(target, params);
        driver.setViewportScissor(0, 0, 1280, 720);
        driver.draw(pipeline, m_primitives[0]);
        driver.endRenderPass();
        driver.flush();
    }
}
