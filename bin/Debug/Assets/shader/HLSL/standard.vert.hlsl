#include "vso.h.hlsl"

cbuffer vpUniforms : register(b0)
{
    float4x4 u_view_matrix;
    float4x4 u_projection_matrix;
    float4 u_camera_pos;
    float4 u_time;
}

cbuffer mUniforms : register(b1)
{
    float4x4 u_model_matrix;
}

// struct a2v_simple {
//     float3 inputPosition : TEXCOORD0;
//     float4 inputColor : TEXCOORD1;
//     float2 inputUV : TEXCOORD2;
// };

//struct appdata
//{
//    float4 vertex : POSITION;
//    float2 uv : TEXCOORD0;
//    float3 normal : NORMAL;
//    float4 tangent : TANGENT;
//#if _SKIN_ON
//    float4 boneWeight : BLENDWEIGHT;
//    float4 boneIndices : BLENDINDICES; 
//#endif
//};

struct appdata
{
    float4 vertex : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
#if _SKIN_ON
    float4 boneWeight : BLENDWEIGHT;
    float4 boneIndices : BLENDINDICES;
#endif
};

v2f vert(appdata a)
{
    v2f o = (v2f)0;
    // o.pos = mul(uProjectionMatrix, mul(uViewMatrix, mul(uWorldMatrix, float4(a.inputPosition.xyz, 1.0))));
    o.vertex = mul(mul(mul(float4(a.vertex.xyz, 1.0), u_model_matrix), u_view_matrix), u_projection_matrix);
    o.vcolor = a.color;
    o.uv = a.uv;
#if COMPILER_HLSL
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
    return o;
}