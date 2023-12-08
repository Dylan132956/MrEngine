#pragma once

#include <vector>

namespace moonriver
{
    class AnimationCurve
    {
    private:
        struct Key
        {
            float time;
            float value;
            float in_tangent;
            float out_tangent;
        };

    public:
        static AnimationCurve Linear(float time_start, float value_start, float time_end, float value_end);
        void AddKey(float time, float value, float in_tangent, float out_tangent);
        float Evaluate(float time) const;

    private:
        static float Evaluate(float time, const Key& k0, const Key& k1);
        
    private:
        std::vector<Key> m_keys;
    };
}
