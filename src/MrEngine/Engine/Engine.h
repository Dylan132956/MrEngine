#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include "thread/ThreadPool.h"

#define VR_VERSION_NAME "1.0.0"

namespace filament
{
    namespace backend
    {
        class CommandStream;
        using DriverApi = CommandStream;
        enum class Backend : uint8_t;
        enum class ShaderModel : uint8_t;
        class Platform;
        class Driver;
    }
}

namespace moonriver
{
    class MrEngine;
    class Scene;
    class Engine
    {
    public:
        static Engine* Create(void* native_window, int width, int height, uint64_t flags = 0, void* shared_gl_context = nullptr);
        static void Destroy(Engine** engine);
        static Engine* Instance();
        void Execute();
        filament::backend::DriverApi& GetDriverApi();
        const filament::backend::Backend& GetBackend() const;
        filament::backend::ShaderModel GetShaderModel() const;
        void* GetDefaultRenderTarget();
        void OnResize(void* native_window, int width, int height, uint64_t flags = 0);
        int GetWidth() const;
        int GetHeight() const;
        bool HasQuit() const;
        ThreadPool* GetThreadPool() const;
        void PostAction(Action action);
        void SendMessage(int id, const std::string& msg);
        void AddMessageHandler(int id, std::function<void(int id, const std::string&)> handler);
        const std::string& GetDataPath();
        void Init();
#if VR_ANDROID
        void SetDataPath(const std::string& path);
        void SetSavePath(const std::string& path);
#endif
        std::shared_ptr<Scene> CreateScene();
    private:
        Engine(void* native_window, int width, int height, uint64_t flags, void* shared_gl_context);
        ~Engine();

    private:
        friend class Memory;

    private:
        static Engine* m_instance;
        MrEngine* m_pCore;
        std::string m_data_path;
    };

    void FreeBufferCallback(void* buffer, size_t size, void* user);
}
