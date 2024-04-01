/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "AudioManager.h"
#include "AudioListener.h"
#include "memory/Memory.h"
#include "Debug.h"
#include "Entity.h"
#include "Scene.h"

#if VR_MAC || VR_IOS
#include <OpenAL/OpenAL.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#if VR_WASM
#include <emscripten.h>

EM_JS(void, PlayAudio, (const char* msg), {
    PlayAudio(UTF8ToString(msg));
});

EM_JS(void, PauseAudio, (const char* msg), {
    PauseAudio(UTF8ToString(msg));
});

EM_JS(void, StopAudio, (const char* msg), {
    StopAudio(UTF8ToString(msg));
});
#endif

namespace moonriver
{
    static ALCdevice* g_device = nullptr;
    static ALCcontext* g_context = nullptr;
    static std::shared_ptr<AudioListener> g_listener;

    void AudioManager::Init()
    {
        std::string name;
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT"))
        {
            name = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        }
        if (name.empty())
        {
            name = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        }
        Log("open al device specifier: %s", name.c_str());

        g_device = alcOpenDevice(nullptr);
        if (g_device == nullptr)
        {
            Log("open al device open failed");
            return;
        }

        g_context = alcCreateContext(g_device, nullptr);
        if (g_context == nullptr)
        {
            Log("open al context create failed");
            return;
        }

        if (alcMakeContextCurrent(g_context) == ALC_FALSE)
        {
            alcDestroyContext(g_context);
            g_context = nullptr;
            alcCloseDevice(g_device);
            g_device = nullptr;
            Log("open al context set failed");
            return;
        }

        Log("open al init success");
    }

    void AudioManager::Done()
    {
        alcMakeContextCurrent(nullptr);
        if (g_context)
        {
            alcDestroyContext(g_context);
            g_context = nullptr;
        }
        if (g_device)
        {
            alcCloseDevice(g_device);
            g_device = nullptr;
        }
    }

    const std::shared_ptr<AudioListener>& AudioManager::GetListener(std::shared_ptr<Scene> pScene)
    {
        if (!g_listener)
        {
            g_listener = pScene->CreateEntity("listener")->AddComponent<AudioListener>();
        }
        return g_listener;
    }

	void AudioManager::DestroyListener()
	{
		g_listener.reset();
	}

#if VR_WASM
    void AudioManager::PlayAudio(const std::string& url, bool loop)
    {
        String msg = String::Format(R"({ "url": "%s", "loop": %s })", url.CString(), loop ? "true" : "false");
        ::PlayAudio(msg.CString());
    }

    void AudioManager::PauseAudio()
    {
        ::PauseAudio("");
    }

    void AudioManager::StopAudio()
    {
        ::StopAudio("");
    }
#endif
}
