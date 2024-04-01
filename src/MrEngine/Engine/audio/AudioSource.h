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

#pragma once

#include "Component.h"

namespace moonriver
{
    class AudioClip;
    class AudioSourcePrivate;

    class AudioSource: public Component
    {
    public:
        enum class State
        {
            Unknown,

            Initial,
            Playing,
            Paused,
            Stopped,
        };

        AudioSource();
        virtual ~AudioSource();
        const std::shared_ptr<AudioClip>& GetClip() const { return m_clip; }
        void SetClip(const std::shared_ptr<AudioClip>& clip);
        void SetLoop(bool loop);
        void Play();
        void Pause();
        void Stop();
        State GetState() const;

    protected:
		virtual void Update();
        virtual void OnTransformDirty();

    private:
        AudioSourcePrivate* m_private;
        std::shared_ptr<AudioClip> m_clip;
    };
}
