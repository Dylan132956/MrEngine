#include "vso.h.hlsl"

cbuffer mvpUniforms : register(b0)
{
    float4x4 uWorldMatrix;
    float4x4 uViewMatrix;
    float4x4 uProjectionMatrix;
}

struct a2v_simple {
    float4 inputPosition : POSITION;
    float4 inColor : COLOR;
};

//static const float4 test = { 1.0, 0.0, 0.0, 1.0};

simple_vert_color_output vert(a2v_simple a)
{
    simple_vert_color_output o;
    o.pos = mul(uProjectionMatrix, mul(uViewMatrix, mul(uWorldMatrix, float4(a.inputPosition.xy, 0.0, 1.0))));
    o.v_color = a.inColor;
    //o.v_color = test;
#if (COMPILER_HLSL == 1)
    o.pos.z = 0.5 * (o.pos.z + o.pos.w);
#endif
#if (COMPILER_VULKAN == 1)
     o.pos.y = -o.pos.y;
     o.pos.z = 0.5 * (o.pos.z + o.pos.w);
#endif

    return o;
}