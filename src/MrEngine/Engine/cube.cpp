#include "cube.h"
#include "Engine.h"
#include "graphics/Shader.h"
#include "io/FileSystem.h"
#include "glslang/Public/ShaderLang.h"
#include "spirv_glsl.hpp"
#include "math/Matrix4x4.h"
#include "math/Vector4.h"
#include "math/Vector2.h"
#include "graphics/Image.h"
//#include "graphics/Texture.h"

void CompileAndLinkShader(EShLanguage stage, const char* text[], const std::string fileName[],
    const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv);
std::string compile_iteration(std::vector<uint32_t>& spirv_file);

namespace moonriver
{
    //const uint32_t cube::mIndices[] = {
    //    // solid
    //    1,0,2, 3,1,2,  // far
    //    6,4,5, 6,5,7,  // near
    //    2,0,4, 2,4,6,  // left
    //    5,1,3, 7,5,3,  // right
    //    5,4,0, 1,5,0,  // bottom
    //    2,6,7, 2,7,3,  // top

    //    // wire-frame
    //    0,1, 1,3, 3,2, 2,0,     // far
    //    4,5, 5,7, 7,6, 6,4,     // near
    //    0,4, 1,5, 3,7, 2,6,
    //};

    const uint32_t cube::mIndices_Tex[] = {
        1,0,2,    0,3,2,
        5,6,4,    6,7,4,
        9,10,8,   10,11,8,
        13,14,12, 14,15,12,
        17,18,16, 18,19,16,
        22,21,20, 23,22,20
    };

    struct Vertex_Cube {
        Vector3 position;
        Vector4 color;
        Vector2 uv;
    };

    //Vertex_Cube CUBE_VERTICES[8] = {
    //    {{-1., -1.,  1.}, {1., 0., 0., 1.}, {0., 1.}},        // 0. left bottom far
    //    {{ 1., -1.,  1.}, {0., 1., 0., 1.}, {1., 1.}},        // 1. right bottom far
    //    {{-1.,  1.,  1.}, {0., 0., 1., 1.}, {0., 0.}},        // 2. left top far
    //    {{ 1.,  1.,  1.}, {1., 0., 0., 1.}, {1., 0.}},        // 3. right top far
    //    {{-1., -1., -1.}, {0., 1., 0., 1.}, {0., 0.}},        // 4. left bottom near
    //    {{ 1., -1., -1.}, {0., 0., 1., 1.}, {1., 0.}},        // 5. right bottom near
    //    {{-1.,  1., -1.}, {1., 0., 0., 1.}, {0., 0.}},        // 6. left top near
    //    {{ 1.,  1., -1.}, {0., 1., 0., 1.}, {1., 1.}}         // 7. right top near
    //};

    const int c_vertexcount = 24;

    // Cube vertices

    //      (-1,+1,+1)________________(+1,+1,+1)
    //               /|              /|
    //              / |             / |
    //             /  |            /  |
    //            /   |           /   |
    //(-1,+1,-1) /____|__________/(+1,+1,-1)
    //           |    |__________|____|
    //           |   /(-1,-1,+1) |    /(+1,-1,+1)
    //           |  /            |   /
    //           | /             |  /
    //           |/              | /
    //           /_______________|/
    //        (-1,-1,-1)       (+1,-1,-1)
    //

    Vertex_Cube CUBE_VERTICES_TEX[c_vertexcount] = {
        //near
        {{-1., -1., -1.}, {1., 0., 0., 1.}, {0., 1.}},
        {{-1., +1., -1.}, {0., 1., 0., 1.}, {0., 0.}},
        {{+1., +1., -1.}, {0., 0., 1., 1.}, {1., 0.}},
        {{+1., -1., -1.}, {1., 0., 0., 1.}, {1., 1.}},
        //bottom
        {{-1., -1., -1.}, {0., 1., 0., 1.}, {1., 1.}},
        {{-1., -1., +1.}, {0., 0., 1., 1.}, {1., 0.}},
        {{+1., -1., +1.}, {1., 0., 0., 1.}, {0., 0.}},
        {{+1., -1., -1.}, {0., 1., 0., 1.}, {0., 1.}},
        //right
        {{+1., -1., -1.}, {0., 1., 0., 1.}, {0., 1.}},
        {{+1., -1., +1.}, {0., 0., 1., 1.}, {1., 1.}},
        {{+1., +1., +1.}, {1., 0., 0., 1.}, {1., 0.}},
        {{+1., +1., -1.}, {0., 1., 0., 1.}, {0., 0.}},
        //top
        {{+1., +1., -1.}, {0., 1., 0., 1.}, {1., 1.}},
        {{+1., +1., +1.}, {0., 0., 1., 1.}, {1., 0.}},
        {{-1., +1., +1.}, {1., 0., 0., 1.}, {0., 0.}},
        {{-1., +1., -1.}, {0., 1., 0., 1.}, {0., 1.}},
        //left
        {{-1., +1., -1.}, {0., 1., 0., 1.}, {1., 0.}},
        {{-1., +1., +1.}, {0., 0., 1., 1.}, {0., 0.}},
        {{-1., -1., +1.}, {1., 0., 0., 1.}, {0., 1.}},
        {{-1., -1., -1.}, {0., 1., 0., 1.}, {1., 1.}},
        //far
        {{-1., -1., +1.}, {0., 1., 0., 1.}, {1., 1.}},
        {{+1., -1., +1.}, {0., 0., 1., 1.}, {0., 1.}},
        {{+1., +1., +1.}, {1., 0., 0., 1.}, {0., 0.}},
        {{-1., +1., +1.}, {0., 1., 0., 1.}, {1., 0.}}
    };

    cube::cube()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        filament::backend::BufferUsage usage = filament::backend::BufferUsage::DYNAMIC;

        int sizes[] = {
            sizeof(Vertex_Cube::position), sizeof(Vertex_Cube::color), sizeof(Vertex_Cube::uv)
        };
        filament::backend::ElementType types[] = {
            filament::backend::ElementType::FLOAT3,
            filament::backend::ElementType::FLOAT4,
            filament::backend::ElementType::FLOAT2
        };
        int offset = 0;
        for (int i = 0; i < (int)Shader::AttributeLocation_Cube::Count; ++i)
        {
            m_attributes[i].offset = offset;
            m_attributes[i].stride = sizeof(Vertex_Cube);
            m_attributes[i].buffer = 0;
            m_attributes[i].type = types[i];
            m_attributes[i].flags = 0;

            offset += sizes[i];
        }
        m_vb = driver.createVertexBuffer(1, (uint8_t)Shader::AttributeLocation_Cube::Count, c_vertexcount, m_attributes, usage);
        driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(CUBE_VERTICES_TEX, c_vertexcount * sizeof(Vertex_Cube), nullptr), 0);
        filament::backend::ElementType index_type = filament::backend::ElementType::UINT;
        m_ib = driver.createIndexBuffer(index_type, 3 * 2 * 6, usage);
        driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(mIndices_Tex, 3 * 2 * 6 * sizeof(mIndices_Tex[0]), nullptr), 0);
        m_primitives.resize(1);
        m_primitives[0] = driver.createRenderPrimitive();
        m_enabled_attributes =
            (1 << (int)Shader::AttributeLocation_Cube::Vertex) |
            (1 << (int)Shader::AttributeLocation_Cube::Color) |
            (1 << (int)Shader::AttributeLocation_Cube::UV);

        driver.setRenderPrimitiveBuffer(m_primitives[0], m_vb, m_ib, m_enabled_attributes);

        driver.setRenderPrimitiveRange(m_primitives[0], filament::backend::PrimitiveType::TRIANGLES, 0, 0, c_vertexcount - 1, 3 * 2 * 6);

        //////////////////////////////////////////////////////////////////////////
        std::string vs_path[1];
        std::string fs_path[1];

        std::string asset_path = Engine::Instance()->GetDataPath();

        vs_path[0] = asset_path + "/shader/HLSL/cube.vert.hlsl";
        fs_path[0] = asset_path + "/shader/HLSL/cube.frag.hlsl";
        char* vs_buffer = FileSystem::ReadFileData(vs_path[0].c_str());
        char* fs_buffer = FileSystem::ReadFileData(fs_path[0].c_str());
        std::string vs_hlsl = vs_buffer;
        std::string fs_hlsl = fs_buffer;
        FileSystem::FreeFileData(vs_buffer);
        FileSystem::FreeFileData(fs_buffer);
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
        }
        else if (Engine::Instance()->GetBackend() == filament::backend::Backend::D3D11)
        {
            vs = "#define COMPILER_HLSL 1\n" + vs_hlsl;
            fs = fs_hlsl;
        }

        std::vector<char> vs_data;
        std::vector<char> fs_data;

        vs_data.resize(vs.size());
        memcpy(&vs_data[0], &vs[0], vs_data.size());
        fs_data.resize(fs.size());
        memcpy(&fs_data[0], &fs[0], fs_data.size());

        filament::backend::Program pb;
        pb.diagnostics(utils::CString("Assets/shader/HLSL/cube"))
            .withVertexShader((void*)&vs_data[0], vs_data.size())
            .withFragmentShader((void*)&fs_data[0], fs_data.size());

        if (!m_cube_sampler_group)
        {
            m_cube_sampler_group = driver.createSamplerGroup(1);
        }

        vector<filament::backend::Program::Sampler> _samplers;

        filament::backend::Program::Sampler sampler;
        sampler.name = utils::CString("SPIRV_Cross_Combinedtex0samp0");
        sampler.binding = 0;
        _samplers.push_back(sampler);
        pb.setSamplerGroup((size_t)4, &_samplers[0], _samplers.size());

        pipeline.program = driver.createProgram(std::move(pb));

        pipeline.rasterState.depthWrite = true;
        pipeline.rasterState.colorWrite = true;
        //disable depthtest
        //pipeline.rasterState.depthFunc = filament::backend::RasterState::DepthFunc::A;
        pipeline.rasterState.culling = filament::backend::RasterState::CullingMode::BACK;
        //pipeline.rasterState.inverseFrontFaces = true;
        m_uniform_buffer = driver.createUniformBuffer(sizeof(mvpUniforms), filament::backend::BufferUsage::DYNAMIC);
        constexpr float ZOOM = 1.5f;
        const float aspect = (float)Engine::Instance()->GetWidth() / Engine::Instance()->GetHeight();
        m_uniforms.uWorldMatrix = Matrix4x4::Identity();
        m_uniforms.uViewMatrix = Matrix4x4::LookTo(Vector3(0., 0., -10.0), Vector3(0., 0., 1.), Vector3(0., 1., 0.));
        //m_uniforms.uProjectionMatrix = Matrix4x4::Ortho(-aspect * ZOOM, aspect * ZOOM, -ZOOM, ZOOM, -20., 20.);

        m_uniforms.uProjectionMatrix = Matrix4x4::Perspective(18., aspect, 0.1, 1000.);
        //test load image jpg png
        //std::shared_ptr<Image> image =  Image::LoadFromFile(asset_path + "/texture/" + "model18.png");
        //image->EncodeToPNG("out.png");

        cube_texture = Texture::LoadTexture2DFromFile(asset_path + "/texture/" + "moonriver.jpg", FilterMode::Linear, SamplerAddressMode::ClampToEdge, false);

        //Shader shader;
    }

    cube::~cube()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        driver.destroyVertexBuffer(m_vb);
        m_vb.clear();

        driver.destroyIndexBuffer(m_ib);
        m_ib.clear();

        if (m_cube_sampler_group)
        {
            driver.destroySamplerGroup(m_cube_sampler_group);
            m_cube_sampler_group.clear();
        }

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

    void cube::Update()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        static float angle = 0.00;
        angle += 0.01;
        m_uniforms.uWorldMatrix = Matrix4x4::RotMat(Vector3(1., 1., 1.), angle) * Matrix4x4::Translation(Vector3(0., 0., 0.));

        void* buffer = driver.allocate(sizeof(mvpUniforms));
        memcpy(buffer, &m_uniforms, sizeof(mvpUniforms));
        driver.loadUniformBuffer(m_uniform_buffer, filament::backend::BufferDescriptor(buffer, sizeof(mvpUniforms)));

        filament::backend::SamplerGroup samplers(1);
        samplers.setSampler(0, cube_texture->GetTexture(), cube_texture->GetSampler());
        driver.updateSamplerGroup(m_cube_sampler_group, std::move(samplers));

        filament::backend::RenderTargetHandle target = *(filament::backend::RenderTargetHandle*)Engine::Instance()->GetDefaultRenderTarget();
        filament::backend::RenderPassParams params;
        params.flags.clear = filament::backend::TargetBufferFlags::COLOR | filament::backend::TargetBufferFlags::DEPTH;

        params.viewport.left = (int32_t)0;
        params.viewport.bottom = (int32_t)0;
        params.viewport.width = (uint32_t)1280;
        params.viewport.height = (uint32_t)720;
        params.clearColor = filament::math::float4(0.22, 0.22, 0.22, 1.0);

        driver.beginRenderPass(target, params);
        driver.setViewportScissor(0, 0, 1280, 720);
        driver.bindUniformBuffer(0, m_uniform_buffer);

        driver.bindSamplers((size_t)4, m_cube_sampler_group);

        driver.draw(pipeline, m_primitives[0]);
        driver.endRenderPass();
        driver.flush();
    }
}
