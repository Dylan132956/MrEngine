#include "Engine.h"
#include <backend/Platform.h>
#include <utils/compiler.h>
#include <utils/CountDownLatch.h>
#include "private/backend/CommandStream.h"
#include "private/backend/CommandBufferQueue.h"
#include "triangle.h"
#include "graphics/Shader.h"
#include "SceneManager.h"
#include "memory/Memory.h"
#include "time/Time.h"
#include "graphics/Texture.h"
#include "graphics/Material.h"
#include "graphics/Mesh.h"
#include "graphics/RenderTarget.h"
#include "graphics/Camera.h"
#include "graphics/Renderer.h"
#include "string/stringutils.h"
#include <thread>
#include "audio/AudioManager.h"
#include "Editor.h"
#include "Input.h"

#include "core/meta/reflection/reflection_register.h"
//#include "core/meta/meta_example.h"

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
    void metaExample();
	struct Message
	{
		int id;
		std::string msg;
	};

	struct MessageHandler
	{
		int id;
		std::function<void(int id, const std::string&)> handler;
	};

    void FreeBufferCallback(void* buffer, size_t size, void* user)
    {
        Memory::Free(buffer, (int)size);
    };

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

        std::unique_ptr<SceneManager> m_scene_manager;

        bool m_quit = false;
		std::shared_ptr<ThreadPool> m_thread_pool;
		std::list<Action> m_actions;
		std::list<Message> m_messages;
		std::map<int, std::list<MessageHandler>> m_message_handlers;
		Mutex m_mutex;
        std::shared_ptr<Editor> m_editor;

        MrEngine(Engine* engine, void* native_window, int width, int height, uint64_t flags, void* shared_gl_context) :
            m_engine(engine),
#if VR_WINDOWS
            m_backend(backend::Backend::D3D12),
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

#if !VR_WASM
			m_thread_pool = std::make_shared<ThreadPool>(4);
#endif
            m_editor = std::make_shared<Editor>();
            Shader::Init();
            AudioManager::Init();
            
            Reflection::TypeMetaRegister::metaRegister();
            //test Reflection system
            metaExample();
		}

        void Shutdown()
        {
            AudioManager::DestroyListener();
            Mesh::Done();
            AudioManager::Done();
            RenderTarget::Done();
            Material::Done();
            Camera::Done();
            Texture::Done();
            Shader::Done();
            m_scene_manager = nullptr;

            m_thread_pool.reset();

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
            Shader::Exit();

            Reflection::TypeMetaRegister::metaUnregister();
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
            Time::SetDrawCall(0);
            Renderer::PrepareAll();
            Camera::RenderAll();
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
#if VR_ANDROID
			if (Input::GetKeyDown(KeyCode::Backspace))
#else
			if (Input::GetKeyDown(KeyCode::Escape))
#endif
			{
				this->Quit();
			}

			Input::Update();
        }

        void Quit()
        {
            m_quit = true;
        }

        void PostAction(Action action)
        {
            m_mutex.lock();
            m_actions.push_back(action);
            m_mutex.unlock();
        }

        void ProcessActions()
        {
            std::list<Action> actions;
            std::list<Message> messages;

            m_mutex.lock();
            actions = m_actions;
            m_actions.clear();
            messages = m_messages;
            m_messages.clear();
            m_mutex.unlock();

            for (const auto& action : actions)
            {
                if (action)
                {
                    action();
                }
            }

            for (const auto& msg : messages)
            {
                if (m_message_handlers.count(msg.id) > 0)
                {
                    const auto& handlers = m_message_handlers[msg.id];
                    for (const auto& handler : handlers)
                    {
                        if (handler.handler)
                        {
                            handler.handler(msg.id, msg.msg);
                        }
                    }
                }
            }
        }

        void SendMessage(int id, const std::string& msg)
        {
            m_mutex.lock();
            m_messages.push_back({ id, msg });
            m_mutex.unlock();
        }

        void AddMessageHandler(int id, std::function<void(int id, const std::string&)> handler)
        {
            m_mutex.lock();

            if (m_message_handlers.count(id) > 0)
            {
                auto& handlers = m_message_handlers[id];
                handlers.push_back({ id, handler });
            }
            else
            {
                std::list<MessageHandler> handlers;
                handlers.push_back({ id, handler });
                m_message_handlers[id] = handlers;
            }

            m_mutex.unlock();
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
        if (m_pCore->m_scene_manager == nullptr) {
            m_pCore->m_scene_manager = std::make_unique<SceneManager>();
        }
        Time::Update();
        m_pCore->m_scene_manager->Update();
        m_pCore->m_editor->Update();
        m_pCore->BeginFrame();
        m_pCore->Render();
        m_pCore->m_scene_manager->Render();
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

    ThreadPool* Engine::GetThreadPool() const
    {
        return m_pCore->m_thread_pool.get();
    }

    void Engine::PostAction(Action action)
    {
        m_pCore->PostAction(action);
    }

    void Engine::SendMessage(int id, const std::string& msg)
    {
        m_pCore->SendMessage(id, msg);
    }

    void Engine::AddMessageHandler(int id, std::function<void(int id, const std::string&)> handler)
    {
        m_pCore->AddMessageHandler(id, handler);
    }

	const std::shared_ptr<Editor>& Engine::GetEditor() const
	{
		return m_pCore->m_editor;
	}

#if VR_WINDOWS
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
#elif VR_ANDROID
	const std::string& Engine::GetDataPath()
	{
		return m_data_path;
	}
	void Engine::SetDataPath(const std::string& path)
	{
		m_data_path = path;
	}
#elif VR_IOS
        const string& Engine::GetDataPath()
        {
            if (m_data_path.empty())
            {
                string path = [[[NSBundle mainBundle] bundlePath] UTF8String];
                path += "/Assets";
                m_data_path = path;
            }
            
            return m_data_path;
        }
        
//        const string& GetSavePath()
//        {
//            if (m_save_path.Empty())
//            {
//                NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
//                NSString* doc_path = paths[0];
//                m_save_path = [doc_path UTF8String];
//            }
//
//            return m_save_path;
//        }
#elif VR_MAC
        const string& Engine::GetDataPath()
        {
            if (m_data_path.empty())
            {
                string path = [[[NSBundle mainBundle] resourcePath] UTF8String];
                path += "/Assets";
                m_data_path = path;
            }
            
            return m_data_path;
        }
        
//        const string& GetSavePath()
//        {
//            if (m_save_path.Empty())
//            {
//                NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
//                NSString* doc_path = [paths objectAtIndex:0];
//                m_save_path = [doc_path UTF8String];
//            }
//
//            return m_save_path;
//        }

#endif
    void Engine::Init()
    {
        m_pCore->m_scene_manager = std::make_unique<SceneManager>();
    }

    std::shared_ptr<Scene> Engine::CreateScene()
    {
        return m_pCore->m_scene_manager->CreateScene();
    }
}
