#include "vso.h.hlsl"

cbuffer mvpUniforms : register(b0)
{
    float4x4 uWorldMatrix;
    float4x4 uViewMatrix;
    float4x4 uProjectionMatrix;
}

struct a2v_simple {
    float3 inputPosition : POSITION;
    float4 inputColor : COLOR;
    float2 inputUV : TEXCOORD0;
};

simple_vert_color_uv_output vert(a2v_simple a)
{
    simple_vert_color_uv_output o;
    //o.pos = mul(uProjectionMatrix, mul(uViewMatrix, mul(uWorldMatrix, float4(a.inputPosition.xyz, 1.0))));
    o.pos = mul(mul(mul(float4(a.inputPosition.xyz, 1.0), uWorldMatrix), uViewMatrix), uProjectionMatrix);
    o.v_color = a.inputColor;
    o.v_uv = a.inputUV;
#if COMPILER_HLSL
    o.pos.z = 0.5 * (o.pos.z + o.pos.w);
#endif
    return o;
}