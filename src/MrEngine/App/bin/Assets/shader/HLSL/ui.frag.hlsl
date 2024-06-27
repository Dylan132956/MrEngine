#include "vso.h.hlsl"

cbuffer PerMaterialFragment : register(b4)
{
    float4 u_color;
}

Texture2D g_ColorMap : reg(t0, space1);
SamplerState g_ColorSampler : reg(s0, space1);

void frag(in v2f _entryPointOutput, out float4 outColor: SV_Target0)
{
    outColor = g_ColorMap.Sample(g_ColorSampler, _entryPointOutput.uv) * _entryPointOutput.vcolor * u_color;
    //outColor = u_color;
    //outColor = float4(1.0, 0.0, 0.0, 1.0);
}