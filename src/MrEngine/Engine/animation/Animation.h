#pragma once

#include "Component.h"
#include "AnimationCurve.h"

namespace moonriver
{
    enum class AnimationCurvePropertyType
    {
        Unknown = 0,

        LocalPositionX,
        LocalPositionY,
        LocalPositionZ,
        LocalRotationX,
        LocalRotationY,
        LocalRotationZ,
        LocalRotationW,
        LocalScaleX,
        LocalScaleY,
        LocalScaleZ,
        BlendShape,
    };
    
    struct AnimationCurveProperty
    {
        AnimationCurvePropertyType type;
        std::string name;
        AnimationCurve curve;
    };

    struct AnimationCurveWrapper
    {
        std::string path;
        std::vector<AnimationCurveProperty> properties;
    };

    enum class AnimationWrapMode
    {
        Default = 0,
        Once = 1,
        Loop = 2,
        PingPong = 4,
        ClampForever = 8,
    };

    class AnimationClip : public Object
    {
	public:
		AnimationClip(): length(0), fps(0), wrap_mode(AnimationWrapMode::Default) { }
		virtual ~AnimationClip() { }

	public:
        std::string name;
        float length;
        float fps;
        AnimationWrapMode wrap_mode;
        std::vector<AnimationCurveWrapper> curves;
    };

    enum class FadeState
    {
        In,
        Normal,
        Out,
    };

    struct AnimationState
    {
        int clip_index;
        float play_start_time;
        std::vector<Transform*> targets;
        FadeState fade_state;
        float fade_start_time;
        float fade_length;
        float start_weight;
        float weight;
        float playing_time;
    };

    class Animation : public Component
    {
    public:
        Animation();
        virtual ~Animation();
		void SetClips(const std::vector<std::shared_ptr<AnimationClip>>& clips);
        int GetClipCount() const { return m_clips.size(); }
        const std::string& GetClipName(int index) const;
        float GetClipLength(int index) const;
        int GetPlayingClip() const;
        float GetPlayingTime() const;
        void SetPlayingTime(float time);
        bool IsPaused() const { return m_paused; }
        bool IsStopped() const { return m_stopped; }
        void Play(int index, float fade_length = 0.3f);
        void Stop();
        void Pause();

    protected:
        virtual void Update();
        
    private:
        void UpdateTime();
        float GetTime();
        void Sample(AnimationState& state, float time, float weight, bool first_state, bool last_state);

    private:
        std::vector<std::shared_ptr<AnimationClip>> m_clips;
        std::list<AnimationState> m_states;
        float m_time = 0;
        float m_seek_to = -1;
        bool m_paused = false;
        bool m_stopped = true;
    };
}
