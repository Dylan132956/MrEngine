#include "cube.h"
#include "Engine.h"
#include "Shader.h"
#include "FileSystem.h"
#include "glslang/Public/ShaderLang.h"
#include "spirv_glsl.hpp"

namespace moonriver
{
    const uint32_t cube::mIndices[] = {
        // solid
        2,0,1, 2,1,3,  // far
        6,4,5, 6,5,7,  // near
        2,0,4, 2,4,6,  // left
        3,1,5, 3,5,7,  // right
        0,4,5, 0,5,1,  // bottom
        2,6,7, 2,7,3,  // top

        // wire-frame
        0,1, 1,3, 3,2, 2,0,     // far
        4,5, 5,7, 7,6, 6,4,     // near
        0,4, 1,5, 3,7, 2,6,
    };

    const filament::math::float3 cube::mVertices[] = {
            { -1/2., -1/2.,  1/2.},  // 0. left bottom far
            {  1/2., -1/2.,  1/2.},  // 1. right bottom far
            { -1/2.,  1/2.,  1/2.},  // 2. left top far
            {  1/2.,  1/2.,  1/2.},  // 3. right top far
            { -1/2., -1/2., -1/2.},  // 4. left bottom near
            {  1/2., -1/2., -1/2.},  // 5. right bottom near
            { -1/2.,  1/2., -1/2.},  // 6. left top near
            {  1/2.,  1/2., -1/2.} }; // 7. right top near

    cube::cube()
    {
        auto& driver = Engine::Instance()->GetDriverApi();
        filament::backend::BufferUsage usage = filament::backend::BufferUsage::DYNAMIC;
        m_attributes[0].offset = 0;
        m_attributes[0].stride = sizeof(float) * 3;
        m_attributes[0].buffer = 0;
        m_attributes[0].type = filament::backend::ElementType::FLOAT3;
        m_vb = driver.createVertexBuffer(1, 1, 8, m_attributes, usage);
        driver.updateVertexBuffer(m_vb, 0, filament::backend::BufferDescriptor(mVertices, 8 * sizeof(mVertices[0]), nullptr), 0);
        filament::backend::ElementType index_type = filament::backend::ElementType::UINT;
        m_ib = driver.createIndexBuffer(index_type, 12 * 2 + 3 * 2 * 6, usage);
        driver.updateIndexBuffer(m_ib, filament::backend::BufferDescriptor(mIndices, 12 * 2 + 3 * 2 * 6 * sizeof(mIndices[0]), nullptr), 0);
        m_primitives.resize(1);
        m_primitives[0] = driver.createRenderPrimitive();
        driver.setRenderPrimitiveBuffer(m_primitives[0], m_vb, m_ib, 1);
        driver.setRenderPrimitiveRange(m_primitives[0], filament::backend::PrimitiveType::TRIANGLES, 0, 0, sizeof(mVertices) - 1, 12 * 2 + 3 * 2 * 6);

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
            vs = vs_hlsl;
            fs = fs_hlsl;
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

    cube::~cube()
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

    void cube::run()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        filament::backend::RenderTargetHandle target = *(filament::backend::RenderTargetHandle*)Engine::Instance()->GetDefaultRenderTarget();
        filament::backend::RenderPassParams params;
        params.flags.clear = filament::backend::TargetBufferFlags::COLOR | filament::backend::TargetBufferFlags::DEPTH;

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
