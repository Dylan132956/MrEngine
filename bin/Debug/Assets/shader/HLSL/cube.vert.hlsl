#include "vso.h.hlsl"

cbuffer mvpUniforms : register(b0)
{
    float4x4 uWorldMatrix;
    float4x4 uViewMatrix;
    float4x4 uProjectionMatrix;
}

struct a2v_simple {
    float3 inputPosition : TEXCOORD0;
    float4 inputColor : TEXCOORD1;
    float2 inputUV : TEXCOORD2;
};

simple_vert_color_uv_output vert(a2v_simple a)
{
    simple_vert_color_uv_output o;
    o.pos = mul(uProjectionMatrix, mul(uViewMatrix, mul(uWorldMatrix, float4(a.inputPosition.xyz, 1.0))));
    o.v_color = a.inputColor;
    o.v_uv = a.inputUV;
#if COMPILER_HLSL
    o.pos.z = 0.5 * (o.pos.z + o.pos.w);
#endif
    return o;
}