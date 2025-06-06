#include "triangle.h"
#include "Engine.h"
#include "graphics/Shader.h"
#include "io/FileSystem.h"
#include "math/Matrix4x4.h"
#include "math/Vector4.h"
#include "math/Vector2.h"
#include "shader_converter.h"
#include "graphics/sprivShader/spirv_shader.h"

// triangle vertices

//               /\(0.0, 0.5)
//              /  \
//             /    \
//            /      \
//           /        \
//          /          \
//         /            \
//        /______________\
// (-0.3, -0.5)      (0.3, -0.5)

// x:right y:up z:front
namespace moonriver
{
    struct Vertex {
        Vector4 position;
        Vector4 color;
    };
    Vertex TRIANGLE_VERTICES[3] = {
        {{0.3, -0.5, 0.0, 1.0}, {1., 0., 0., 1.}},
        {{0.0, 0.5, 0.0, 1.0}, {0., 1., 0., 1.}},
        {{-0.3, -0.5, 0.0, 1.0}, { 0., 0., 1., 1.}}
    };
    static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

    triangle::triangle()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        filament::backend::BufferUsage usage = filament::backend::BufferUsage::DYNAMIC;
        m_attributes[0].offset = 0;
        m_attributes[0].stride = sizeof(Vertex);
        m_attributes[0].buffer = 0;
        m_attributes[0].type = filament::backend::ElementType::FLOAT4;
        m_attributes[0].flags = 0;
        m_attributes[0].Semantic = 0;

        m_attributes[1].offset = sizeof(float) * 4;
        m_attributes[1].stride = sizeof(Vertex);
        m_attributes[1].buffer = 0;
        m_attributes[1].type = filament::backend::ElementType::FLOAT4;
        m_attributes[1].flags = 0;
        m_attributes[1].Semantic = 4;

        m_vb = driver.createVertexBuffer(1, 2, 3, m_attributes, usage);
        driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(TRIANGLE_VERTICES, sizeof(Vertex) * 3, nullptr), 0);
        filament::backend::ElementType index_type = filament::backend::ElementType::USHORT;
        m_ib = driver.createIndexBuffer(index_type, 3, usage);
        driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr), 0);
        m_primitives.resize(1);
        m_primitives[0] = driver.createRenderPrimitive();

        driver.setRenderPrimitiveBuffer(m_primitives[0], m_vb, m_ib, 3);
        driver.setRenderPrimitiveRange(m_primitives[0], filament::backend::PrimitiveType::TRIANGLES, 0, 0, 2, 3);

        //////////////////////////////////////////////////////////////////////////
        std::string vs_path[1];
        std::string fs_path[1];

        std::string asset_path = Engine::Instance()->GetDataPath();

        vs_path[0] = asset_path + "/shader/HLSL/triangle.vert.hlsl";
        fs_path[0] = asset_path + "/shader/HLSL/triangle.frag.hlsl";
        char* vs_buffer = FileSystem::ReadFileData(vs_path[0].c_str());
        char* fs_buffer = FileSystem::ReadFileData(fs_path[0].c_str());
        std::string vs_hlsl = vs_buffer;
        std::string fs_hlsl = fs_buffer;
        FileSystem::FreeFileData(vs_buffer);
        FileSystem::FreeFileData(fs_buffer);
        //////////////////////////////////////////////////////////////////////////
        std::vector<char> vs_data;
        std::vector<char> fs_data;
        if (Engine::Instance()->GetBackend() == filament::backend::Backend::OPENGL) {
            const char* c_vs_hlsl[1];
            const char* c_fs_hlsl[1];

            vs_hlsl = "#define COMPILER_HLSL 0\n" + vs_hlsl;

            c_vs_hlsl[0] = vs_hlsl.c_str();
            c_fs_hlsl[0] = fs_hlsl.c_str();

            const char* c_vs_path[1];
            const char* c_fs_path[1];
            c_vs_path[0] = vs_path[0].c_str();
            c_fs_path[0] = fs_path[0].c_str();

            std::vector<unsigned int> vs_spriv;
            int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
            std::string entryPointName = "vert";
            converter_spirv(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

            std::vector<unsigned int> fs_spriv;
            option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
            entryPointName = "frag";
            converter_spirv(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

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
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11 || Engine::Instance()->GetBackend() == filament::backend::Backend::D3D12)
        {
            std::string vs = "#define COMPILER_HLSL 1\n" + vs_hlsl;
            std::string fs = fs_hlsl;

            vs_data.resize(vs.size());
            memcpy(&vs_data[0], &vs[0], vs_data.size());
            fs_data.resize(fs.size());
            memcpy(&fs_data[0], &fs[0], fs_data.size());
        }
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::VULKAN)
        {
            const char* c_vs_hlsl[1];
            const char* c_fs_hlsl[1];

            vs_hlsl = "#define COMPILER_VULKAN 1\n" + vs_hlsl;

            c_vs_hlsl[0] = vs_hlsl.c_str();
            c_fs_hlsl[0] = fs_hlsl.c_str();

            const char* c_vs_path[1];
            const char* c_fs_path[1];
            c_vs_path[0] = vs_path[0].c_str();
            c_fs_path[0] = fs_path[0].c_str();

            std::vector<unsigned int> vs_spriv;
            int option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
            std::string entryPointName = "vert";
            converter_spirv(EShLangVertex, c_vs_hlsl, vs_path, c_vs_path, entryPointName.c_str(), 1, option, vs_spriv);

            std::vector<unsigned int> fs_spriv;
            option = (1 << 11) | (1 << 13) | (1 << 5) | (1 << 17);
            entryPointName = "frag";
            converter_spirv(EShLangFragment, c_fs_hlsl, fs_path, c_fs_path, entryPointName.c_str(), 1, option, fs_spriv);

            //std::string vs_glsl = compile_iteration(vs_spriv);

            //std::string fs_glsl = compile_iteration(fs_spriv);

            vs_data.resize(vs_spriv.size() * 4);
            memcpy(&vs_data[0], &vs_spriv[0], vs_data.size());
            fs_data.resize(fs_spriv.size() * 4);
            memcpy(&fs_data[0], &fs_spriv[0], fs_data.size());
        }

		std::string shaderPath = asset_path + "/shader/HLSL/triangle";
        filament::backend::Program pb;
        pb.diagnostics(utils::CString(shaderPath.c_str()))
            .withVertexShader((void*)&vs_data[0], vs_data.size())
            .withFragmentShader((void*)&fs_data[0], fs_data.size());

        //uniform
        pb.setUniformBlock(0, utils::CString("mvpUniforms"));

        pipeline.program = driver.createProgram(std::move(pb));

        pipeline.rasterState.depthWrite = true;
        pipeline.rasterState.colorWrite = true;
        //disable depthtest
        pipeline.rasterState.depthFunc = filament::backend::RasterState::DepthFunc::A;
        pipeline.rasterState.culling = filament::backend::RasterState::CullingMode::NONE;

        m_uniform_buffer = driver.createUniformBuffer(sizeof(mvpUniforms), filament::backend::BufferUsage::DYNAMIC);

        constexpr float ZOOM = 1.5f;
        const float aspect = (float)Engine::Instance()->GetWidth() / Engine::Instance()->GetHeight();

        m_uniforms.uWorldMatrix = Matrix4x4::Identity();
        m_uniforms.uViewMatrix = Matrix4x4::LookTo(Vector3(0., 0., -1.), Vector3(0., 0., 1.), Vector3(0., 1., 0.));
        m_uniforms.uProjectionMatrix = Matrix4x4::Ortho(-aspect * ZOOM, aspect * ZOOM, -ZOOM, ZOOM, 0., 10.);
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

        if (m_uniform_buffer)
        {
            driver.destroyUniformBuffer(m_uniform_buffer);
            m_uniform_buffer.clear();
        }
    }

    void triangle::Render()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        filament::backend::RenderTargetHandle target = *(filament::backend::RenderTargetHandle*)Engine::Instance()->GetDefaultRenderTarget();
        filament::backend::RenderPassParams params;
        params.flags.clear = filament::backend::TargetBufferFlags::COLOR;//| filament::backend::TargetBufferFlags::DEPTH;

        params.viewport.left = (int32_t)0;
        params.viewport.bottom = (int32_t)0;
        params.viewport.width = Engine::Instance()->GetWidth();
        params.viewport.height = Engine::Instance()->GetHeight();
        params.clearColor = filament::math::float4(0.22, 0.22, 0.22, 1.0);

        static float angle = 0.01;
        angle += 0.01;
        Vector3 r = Vector3(1., 1., 1.);
        m_uniforms.uWorldMatrix = Matrix4x4::RotMat(r, angle);

        void* buffer = driver.allocate(sizeof(mvpUniforms));
        memcpy(buffer, &m_uniforms, sizeof(mvpUniforms));
        driver.loadUniformBuffer(m_uniform_buffer, filament::backend::BufferDescriptor(buffer, sizeof(mvpUniforms)));

        driver.beginRenderPass(target, params);
        driver.setViewportScissor(0, 0, Engine::Instance()->GetWidth(), Engine::Instance()->GetHeight());

        driver.bindUniformBuffer(0, m_uniform_buffer);

        driver.draw(pipeline, m_primitives[0]);
        driver.endRenderPass();
        driver.flush();
    }
}
