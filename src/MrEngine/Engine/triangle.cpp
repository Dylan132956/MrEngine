#include "triangle.h"
#include "Engine.h"
#include "Shader.h"

static const filament::math::float2 TRIANGLE_VERTICES[3] = { {1, 0}, {-0.5, 0.866}, {-0.5, -0.866} };
static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

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

        std::string vs = R"(
            attribute vec4 i_vertex;
            void main()
            {
	            gl_Position = vec4(i_vertex.xy, 0.0, 1.0);
            }
            )";
        std::string fs = R"(
            //#extension GL_ARB_gpu_shader5: enable
            void main()
            {
	            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
            )";

        std::vector<char> vs_data;
        std::vector<char> fs_data;

        vs_data.resize(vs.size());
        memcpy(&vs_data[0], &vs[0], vs_data.size());
        fs_data.resize(fs.size());
        memcpy(&fs_data[0], &fs[0], fs_data.size());

        filament::backend::Program pb;
        pb.diagnostics(utils::CString("triangle"))
            .withVertexShader((void*)&vs_data[0], vs_data.size())
            .withFragmentShader((void*)&fs_data[0], fs_data.size());

        pipeline.program = driver.createProgram(std::move(pb));

        pipeline.rasterState.depthWrite = false;
        pipeline.rasterState.colorWrite = true;
        //disable depthtest
        pipeline.rasterState.depthFunc = filament::backend::RasterState::DepthFunc::A;


        Shader shader;
    }

    triangle::~triangle()
    {

    }

    void triangle::run()
    {
        auto& driver = Engine::Instance()->GetDriverApi();

        filament::backend::RenderTargetHandle target = *(filament::backend::RenderTargetHandle*)Engine::Instance()->GetDefaultRenderTarget();
        filament::backend::RenderPassParams params;
        params.flags.clear = filament::backend::TargetBufferFlags::COLOR;// | filament::backend::TargetBufferFlags::DEPTH;

        params.viewport.left = (int32_t)0;
        params.viewport.bottom = (int32_t)0;
        params.viewport.width = (uint32_t)1280;
        params.viewport.height = (uint32_t)720;
        params.clearColor = filament::math::float4(0.0, 1.0, 0.0, 1.0);

        driver.beginRenderPass(target, params);
        driver.draw(pipeline, m_primitives[0]);
        driver.endRenderPass();
        driver.flush();
       
    }
}
