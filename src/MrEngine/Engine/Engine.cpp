#include "Engine.h"
#include <backend/Platform.h>
#include <utils/compiler.h>
#include <utils/CountDownLatch.h>
#include "private/backend/CommandStream.h"
#include "private/backend/CommandBufferQueue.h"
#include "triangle.h"
#include "Shader.h"

#if VR_WINDOWS
#include <Windows.h>
#undef SendMessage
#elif VR_IOS
#import <UIKit/UIKit.h>
#elif VR_MAC
#import <Cocoa/Cocoa.h>
#elif VR_ANDROID
#include "android/jni.h"
#endif

//#undef UTILS_HAS_THREADING
//#define UTILS_HAS_THREADING 0

using namespace filament;

namespace moonriver
{
    class MrEngine
    {
    public:
        static constexpr size_t CONFIG_MIN_COMMAND_BUFFERS_SIZE = 1 * 1024 * 1024;
        static constexpr size_t CONFIG_COMMAND_BUFFERS_SIZE = 3 * CONFIG_MIN_COMMAND_BUFFERS_SIZE;

        Engine* m_engine;
        backend::Backend m_backend;
        backend::Platform* m_platform = nullptr;
        void* m_shared_gl_context = nullptr;
        std::thread m_driver_thread;
        utils::CountDownLatch m_driver_barrier;
        utils::CountDownLatch m_frame_barrier;
        backend::Driver* m_driver = nullptr;
        backend::CommandBufferQueue m_command_buffer_queue;
        backend::DriverApi m_command_stream;
        void* m_native_window;
        int m_width;
        int m_height;
        uint64_t m_window_flags;
        backend::SwapChainHandle m_swap_chain;
        uint32_t m_frame_id = 0;
        backend::RenderTargetHandle m_render_target;

        bool m_quit = false;

        MrEngine(Engine* engine, void* native_window, int width, int height, uint64_t flags, void* shared_gl_context) :
            m_engine(engine),
#if VR_WINDOWS
            m_backend(backend::Backend::OPENGL),
#elif VR_UWP
            m_backend(backend::Backend::D3D11),
#elif VR_ANDROID
            m_backend(backend::Backend::OPENGL),
#elif VR_USE_METAL
            m_backend(backend::Backend::METAL),
#else
            m_backend(backend::Backend::OPENGL),
#endif
            m_shared_gl_context(shared_gl_context),
            m_driver_barrier(1),
            m_frame_barrier(1),
            m_command_buffer_queue(CONFIG_MIN_COMMAND_BUFFERS_SIZE, CONFIG_COMMAND_BUFFERS_SIZE),
            m_native_window(native_window),
            m_width(width),
            m_height(height),
            m_window_flags(flags)
        {

        }

        ~MrEngine()
        {
            if (m_driver)
            {
                delete m_driver;
            }

            backend::DefaultPlatform::destroy((backend::DefaultPlatform**)&m_platform);
        }

        backend::DriverApi& GetDriverApi() { return m_command_stream; }

        void Init()
        {
            m_command_stream = backend::CommandStream(*m_driver, m_command_buffer_queue.getCircularBuffer());
            m_swap_chain = this->GetDriverApi().createSwapChain(m_native_window, m_window_flags);
            m_render_target = this->GetDriverApi().createDefaultRenderTarget();

            Shader::Init();
        }

        void Shutdown()
        {
            this->GetDriverApi().destroyRenderTarget(m_render_target);

            if (!UTILS_HAS_THREADING)
            {
                this->Execute();
            }

            this->GetDriverApi().destroySwapChain(m_swap_chain);

            this->Flush();
            if (!UTILS_HAS_THREADING)
            {
                this->Execute();
            }
            m_command_buffer_queue.requestExit();

            if (UTILS_HAS_THREADING)
            {
                m_driver_thread.join();
            }
        }

        void Loop()
        {
            m_platform = backend::DefaultPlatform::create(&m_backend);
            m_driver = m_platform->createDriver(m_shared_gl_context);
            m_driver_barrier.latch();
            if (!m_driver)
            {
                return;
            }

            while (true)
            {
                if (!this->Execute())
                {
                    break;
                }
            }

            this->GetDriverApi().terminate();
        }

        bool Execute()
        {
            auto buffers = m_command_buffer_queue.waitForCommands();
            if (buffers.empty())
            {
                return false;
            }

            for (int i = 0; i < buffers.size(); ++i)
            {
                auto& item = buffers[i];
                if (item.begin)
                {
                    m_command_stream.execute(item.begin);
                    m_command_buffer_queue.releaseBuffer(item);
                }
            }

            return true;
        }

        void Flush()
        {
            m_driver->purge();
            m_command_buffer_queue.flush();
        }

        void BeginFrame()
        {
            ++m_frame_id;

            auto& driver = this->GetDriverApi();
            driver.makeCurrent(m_swap_chain, m_swap_chain);

            int64_t monotonic_clock_ns(std::chrono::steady_clock::now().time_since_epoch().count());
            driver.beginFrame(monotonic_clock_ns, m_frame_id);
        }

        void Render()
        {
            static triangle t;
            t.run();

            this->Flush();
        }

        void EndFrame()
        {
            this->GetDriverApi().commit(m_swap_chain);
            this->GetDriverApi().endFrame(m_frame_id);
            if (UTILS_HAS_THREADING)
            {
                this->GetDriverApi().queueCommand([this]() {
                    m_frame_barrier.latch();
                    });
            }
            this->Flush();

            if (UTILS_HAS_THREADING)
            {
                m_frame_barrier.await();
                m_frame_barrier.reset(1);
            }
        }

        void Quit()
        {
            m_quit = true;
        }

        void OnResize(void* native_window, int width, int height, uint64_t flags)
        {
            this->GetDriverApi().destroyRenderTarget(m_render_target);
            this->GetDriverApi().destroySwapChain(m_swap_chain);

            m_native_window = native_window;
            m_width = width;
            m_height = height;
            m_window_flags = flags;

            m_swap_chain = this->GetDriverApi().createSwapChain(m_native_window, m_window_flags);
            m_render_target = this->GetDriverApi().createDefaultRenderTarget();
        }

    };

    Engine* Engine::m_instance = nullptr;

    Engine* Engine::Create(void* native_window, int width, int height, uint64_t flags, void* shared_gl_context)
    {
        Engine* instance = new Engine(native_window, width, height, flags, shared_gl_context);
        m_instance = instance;

        if (!UTILS_HAS_THREADING)
        {
            instance->m_pCore->m_platform = backend::DefaultPlatform::create(&instance->m_pCore->m_backend);
            instance->m_pCore->m_driver = instance->m_pCore->m_platform->createDriver(instance->m_pCore->m_shared_gl_context);
            if (!instance->m_pCore->m_driver)
            {
                delete instance;
                m_instance = nullptr;
                return nullptr;
            }
            instance->m_pCore->Init();
            instance->m_pCore->Execute();
        }
        else
        {
            instance->m_pCore->m_driver_thread = std::thread(&MrEngine::Loop, instance->m_pCore);
            instance->m_pCore->m_driver_barrier.await();
            if (!instance->m_pCore->m_driver)
            {
                instance->m_pCore->m_driver_thread.join();
                delete instance;
                m_instance = nullptr;
                return nullptr;
            }
            instance->m_pCore->Init();
        }

        return instance;
    }

    void Engine::Destroy(Engine** engine)
    {
        if (engine)
        {
            if (*engine)
            {
                (*engine)->m_pCore->Shutdown();
                delete *engine;
                m_instance = nullptr;
            }
        }
    }

    Engine* Engine::Instance()
    {
        return m_instance;
    }

    Engine::Engine(void* native_window, int width, int height, uint64_t flags, void* shared_gl_context) :
        m_pCore(new MrEngine(this, native_window, width, height, flags, shared_gl_context))
    {

    }

    Engine::~Engine()
    {
        delete m_pCore;
    }

    void Engine::Execute()
    {
        m_pCore->BeginFrame();
        m_pCore->Render();
        m_pCore->EndFrame();

        if (!UTILS_HAS_THREADING)
        {
            m_pCore->Flush();
            m_pCore->Execute();
        }
    }

    backend::DriverApi& Engine::GetDriverApi()
    {
        return m_pCore->GetDriverApi();
    }

    const backend::Backend& Engine::GetBackend() const
    {
        return m_pCore->m_backend;
    }

    backend::ShaderModel Engine::GetShaderModel() const
    {
        return m_pCore->m_driver->getShaderModel();
    }

    void* Engine::GetDefaultRenderTarget()
    {
        return &m_pCore->m_render_target;
    }

    void Engine::OnResize(void* native_window, int width, int height, uint64_t flags)
    {
        m_pCore->OnResize(native_window, width, height, flags);
    }

    int Engine::GetWidth() const
    {
        return m_pCore->m_width;
    }

    int Engine::GetHeight() const
    {
        return m_pCore->m_height;
    }

    bool Engine::HasQuit() const
    {
        return m_pCore->m_quit;
    }

    std::string Replace(const std::string& input, const std::string& old, const std::string& to)
    {
        std::string result(input);

        int start = 0;
        while (true)
        {
            int index = result.find(old, start);
            if (index >= 0)
            {
                result.replace(index, old.size(), to);
                start = index + (int)to.size();
            }
            else
            {
                break;
            }
        }

        return result;
    }

    const std::string& Engine::GetDataPath()
    {
        if (m_data_path.empty())
        {
            char buffer[MAX_PATH];
            ::GetModuleFileName(nullptr, buffer, MAX_PATH);
            std::string path = buffer;
            path = Replace(path, "\\", "/").substr(0, path.rfind("\\")) + "/Assets";
            m_data_path = path;
        }

        return m_data_path;
    }
}