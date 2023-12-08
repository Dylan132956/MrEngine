#pragma once

#include <string>

namespace moonriver
{
	struct Color
	{
		static const Color& White();
		static const Color& Black();
		static const Color& Red();
		static const Color& Green();
		static const Color& Blue();

		static Color Lerp(const Color &from, const Color &to, float t, bool clamp_01 = true);

		Color(float r = 1, float g = 1, float b = 1, float a = 1);
		bool operator ==(const Color &c) const;
		bool operator !=(const Color &c) const;
		Color operator *(const Color &c) const;
		Color &operator *=(const Color &c);
		Color operator *(float v) const;
		Color operator /(float v) const;

        template <typename Y>
        static Color MakeVector(const Y& vals)
        {
            return Color //
            {
                static_cast<float>(vals[0]),
                static_cast<float>(vals[1]),
                static_cast<float>(vals[2]),
                static_cast<float>(vals[3]),
            };
        }

		float r;
		float g;
		float b;
		float a;
	};
}
