#include "vso.h.hlsl"

cbuffer vpUniforms : register(b0)
{
    float4x4 u_view_matrix;
    float4x4 u_projection_matrix;
}

cbuffer mUniforms : register(b1)
{
    float4x4 u_model_matrix;
}

cbuffer vertexUniforms : register(b3)
{
    float4 u_texture_scale_offset;
}

struct appdata
{
    float4 vertex : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
#if (SKIN_ON == 1)
    float4 boneWeight : BLENDWEIGHT;
    float4 boneIndices : BLENDINDICES;
#endif
};

v2f vert(appdata a)
{
    v2f o = (v2f)0;
    float4x4 model_matrix = u_model_matrix;
    o.vertex = mul(mul(mul(float4(a.vertex.xyz, 1.0), model_matrix), u_view_matrix), u_projection_matrix);
    o.vcolor = a.color;
    o.uv = a.uv * u_texture_scale_offset.xy + u_texture_scale_offset.zw;

#if (COMPILER_HLSL == 1)
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
#if (COMPILER_VULKAN == 1)
    o.vertex.y = -o.vertex.y;
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
    return o;
}