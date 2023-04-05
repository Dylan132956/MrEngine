#include "vsoutput.h.hlsl"

#define _FRESNELAPLPHA_ON 1

struct a2v_simple {
    float3 inputPosition : POSITION;
    float2 inputUV : TEXCOORD;
};

cbuffer PerFrameConstants : register(b0) {
    row_major float4x4 modelMatrix;        // 64 bytes
    row_major float4x4 viewMatrix;        // 64 bytes
    row_major float4x4 projectionMatrix;  // 64 bytes
};                                // totle 192 bytes

simple_vert_output simple_vert_main(a2v_simple a)
{
    simple_vert_output o;

    o.pos = mul(mul(mul(float4(a.inputPosition, 1.0f), modelMatrix), viewMatrix), projectionMatrix);
    o.uv = a.inputUV;
#if _FRESNELAPLPHA_ON
    o.uv = float2(0.0, 1.0);
#endif

    return o;
}
