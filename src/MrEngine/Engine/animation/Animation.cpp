
#include "Animation.h"
#include "Entity.h"
#include "time/Time.h"
#include "math/Mathf.h"
#include "graphics/SkinnedMeshRenderer.h"
#include "Transform.h"
#include "Debug.h"

namespace moonriver
{
    Animation::Animation()
    {
    
    }

    Animation::~Animation()
    {
    
    }

	void Animation::SetClips(const std::vector<std::shared_ptr<AnimationClip>>& clips)
	{
		m_clips = clips;

		m_states.clear();
	}

    const std::string& Animation::GetClipName(int index) const
    {
        return m_clips[index]->name;
    }

    float Animation::GetClipLength(int index) const
    {
        return m_clips[index]->length;
    }

    int Animation::GetPlayingClip() const
    {
        if (m_states.size() == 0 || m_stopped)
        {
            return -1;
        }

        return m_states.back().clip_index;
    }

    float Animation::GetPlayingTime() const
    {
        if (m_states.size() == 0 || m_stopped)
        {
            return 0;
        }

        return m_states.back().playing_time;
    }

    void Animation::SetPlayingTime(float time)
    {
        int playing_clip = this->GetPlayingClip();
        if (playing_clip >= 0)
        {
            time = Mathf::Clamp(time, 0.0f, this->GetClipLength(playing_clip));
            float offset = time - this->GetPlayingTime();
            m_seek_to = m_time + offset;
        }
    }

    float Animation::GetTime()
    {
        return m_time;
    }

    void Animation::Play(int index, float fade_length)
    {
        if (m_paused)
        {
            m_paused = false;

            if (index == this->GetPlayingClip())
            {
                return;
            }
        }

        if (m_states.size() == 0)
        {
            fade_length = 0;
        }
        
        if (fade_length > 0.0f)
        {
            for (auto& state : m_states)
            {
                state.fade_state = FadeState::Out;
                state.fade_start_time = this->GetTime();
                state.fade_length = fade_length;
                state.start_weight = state.weight;
            }
        }
        else
        {
            m_states.clear();
        }

        AnimationState state;
        state.clip_index = index;
        state.play_start_time = this->GetTime();
        state.fade_start_time = this->GetTime();
        state.fade_length = fade_length;
        if (fade_length > 0.0f)
        {
            state.fade_state = FadeState::In;
            state.start_weight = 0.0f;
            state.weight = 0.0f;
        }
        else
        {
            state.fade_state = FadeState::Normal;
            state.start_weight = 1.0f;
            state.weight = 1.0f;
        }
        state.playing_time = 0.0f;

        m_states.push_back(state);
        m_paused = false;
        m_stopped = false;
    }

    void Animation::Stop()
    {
        m_paused = false;
        m_stopped = true;
    }

    void Animation::Pause()
    {
        if (!m_stopped)
        {
            m_paused = true;
        }
    }

    void Animation::UpdateTime()
    {
        if (m_seek_to >= 0)
        {
            m_time = m_seek_to;
            m_seek_to = -1;
        }
        else
        {
            if (!m_paused)
            {
                m_time += Time::GetDeltaTime();
            }
        }
    }

    void Animation::Update()
    {
        this->UpdateTime();

        bool first_state = true;

        for (auto i = m_states.begin(); i != m_states.end(); )
        {
            auto& state = *i;
            float time = this->GetTime() - state.play_start_time;
            const auto& clip = *m_clips[state.clip_index];
            bool remove_later = false;
			
            if (time >= clip.length)
            {
                switch (clip.wrap_mode)
                {
                    case AnimationWrapMode::Once:
                        time = clip.length;
                        remove_later = true;
                        break;
                    case AnimationWrapMode::Loop:
                        time = fmod(time, clip.length);
                        break;
                    case AnimationWrapMode::PingPong:
                        if ((int) (time / clip.length) % 2 == 1)
                        {
                            // backward
                            time = clip.length - fmod(time, clip.length);
                        }
                        else
                        {
                            // forward
                            time = fmod(time, clip.length);
                        }
                        break;
                    case AnimationWrapMode::Default:
                    case AnimationWrapMode::ClampForever:
                        time = clip.length;
                        break;
                }
            }
            state.playing_time = time;

            if (m_stopped)
            {
                state.playing_time = 0;
            }

            float fade_time = this->GetTime() - state.fade_start_time;
            switch (state.fade_state)
            {
                case FadeState::In:
                    if (fade_time < state.fade_length)
                    {
                        state.weight = Mathf::Lerp(state.start_weight, 1.0f, fade_time / state.fade_length);
                    }
                    else
                    {
                        state.fade_state = FadeState::Normal;
                        state.fade_start_time = this->GetTime();
                        state.start_weight = 1.0f;
                        state.weight = 1.0f;
                    }

                    if (m_stopped)
                    {
                        state.fade_state = FadeState::Normal;
                        state.fade_start_time = this->GetTime();
                        state.start_weight = 1.0f;
                        state.weight = 1.0f;
                    }
                    break;
                case FadeState::Normal:
                    break;
                case FadeState::Out:
                    if (fade_time < state.fade_length)
                    {
                        state.weight = Mathf::Lerp(state.start_weight, 0.0f, fade_time / state.fade_length);
                    }
                    else
                    {
                        state.weight = 0.0f;
                        remove_later = true;
                    }

                    if (m_stopped)
                    {
                        state.weight = 0.0f;
                        remove_later = true;
                    }
                    break;
            }

            bool last_state = false;
            auto j = i;
            if (++j == m_states.end())
            {
                last_state = true;
            }

            this->Sample(state, state.playing_time, state.weight, first_state, last_state);
            first_state = false;

            if (remove_later)
            {
                i = m_states.erase(i);

                if (m_states.empty())
                {
                    m_paused = true;
                }
            }
            else
            {
                ++i;
            }
        }

        if (m_stopped)
        {
            m_states.clear();
        }
    }

    void Animation::Sample(AnimationState& state, float time, float weight, bool first_state, bool last_state)
    {
        const auto& clip = *m_clips[state.clip_index];
        if (state.targets.size() == 0)
        {
            state.targets.resize(clip.curves.size(), nullptr);
        }

        for (int i = 0; i < clip.curves.size(); ++i)
        {
            const auto& curve = clip.curves[i];
            Transform* target = state.targets[i];
            if (target == nullptr)
            {
                auto find = this->GetTransform()->Find(curve.path);
                if (find)
                {
                    target = find.get();
                    state.targets[i] = target;
                }
                else
                {
                    continue;
                }
            }

            Vector3 local_pos;
            Quaternion local_rot;
            Vector3 local_scale;
            bool set_pos = false;
            bool set_rot = false;
            bool set_scale = false;

            for (int j = 0; j < curve.properties.size(); ++j)
            {
                auto type = curve.properties[j].type;
                float value = curve.properties[j].curve.Evaluate(time);

                switch (type)
                {
                    case AnimationCurvePropertyType::LocalPositionX:
                        local_pos.x = value;
                        set_pos = true;
                        break;
                    case AnimationCurvePropertyType::LocalPositionY:
                        local_pos.y = value;
                        set_pos = true;
                        break;
                    case AnimationCurvePropertyType::LocalPositionZ:
                        local_pos.z = value;
                        set_pos = true;
                        break;

                    case AnimationCurvePropertyType::LocalRotationX:
                        local_rot.x = value;
                        set_rot = true;
                        break;
                    case AnimationCurvePropertyType::LocalRotationY:
                        local_rot.y = value;
                        set_rot = true;
                        break;
                    case AnimationCurvePropertyType::LocalRotationZ:
                        local_rot.z = value;
                        set_rot = true;
                        break;
                    case AnimationCurvePropertyType::LocalRotationW:
                        local_rot.w = value;
                        set_rot = true;
                        break;

                    case AnimationCurvePropertyType::LocalScaleX:
                        local_scale.x = value;
                        set_scale = true;
                        break;
                    case AnimationCurvePropertyType::LocalScaleY:
                        local_scale.y = value;
                        set_scale = true;
                        break;
                    case AnimationCurvePropertyType::LocalScaleZ:
                        local_scale.z = value;
                        set_scale = true;
                        break;
                        
                    case AnimationCurvePropertyType::BlendShape:
					{
						auto skin = target->GetEntity()->GetComponent<SkinnedMeshRenderer>();
						if (skin)
						{
							std::string blend_shape_name = curve.properties[j].name.substr(std::string("blendShape.").size());
							skin->SetBlendShapeWeight(blend_shape_name, value / 100.0f);
						}
						break;
					}
                    case AnimationCurvePropertyType::Unknown:
                        break;
                }
            }

            if (set_pos)
            {
                Vector3 pos;
                if (first_state)
                {
                    pos = local_pos * weight;
                }
                else
                {
                    pos = target->GetLocalPosition() + local_pos * weight;
                }
                target->SetLocalPosition(pos);
            }
            if (set_rot)
            {
                Quaternion rot;
                if (first_state)
                {
                    rot = local_rot * weight;
                }
                else
                {
                    rot = Quaternion(target->GetLocalRotation());
                    if (rot.Dot(local_rot) < 0)
                    {
                        local_rot = local_rot * -1.0f;
                    }
                    rot.x += local_rot.x * weight;
                    rot.y += local_rot.y * weight;
                    rot.z += local_rot.z * weight;
                    rot.w += local_rot.w * weight;
                }
                if (last_state)
                {
                    rot.Normalize();
                }
                target->SetLocalRotation(rot);
            }
            if (set_scale)
            {
                Vector3 scale;
                if (first_state)
                {
                    scale = local_scale * weight;
                }
                else
                {
                    scale = target->GetLocalScale() + local_scale * weight;
                }
                target->SetLocalScale(scale);
            }
        }
    }
}
