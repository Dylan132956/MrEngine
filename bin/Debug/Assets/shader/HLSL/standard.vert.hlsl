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

#if (SKIN_ON == 1)
cbuffer boneUniforms : register(b2)
{
    float4 u_bones[210];
};
#endif
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
#if (SKIN_ON == 1)
    float4 boneWeight : BLENDWEIGHT;
    float4 boneIndices : BLENDINDICES;
#endif
};

//#if (_SKIN_ON == 1)
//float4x4 skin_mat()
//{
//    int index_0 = int(boneIndices.x);
//    int index_1 = int(boneIndices.y);
//    int index_2 = int(boneIndices.z);
//    int index_3 = int(boneIndices.w);
//    float weights_0 = boneWeight.x;
//    float weights_1 = boneWeight.y;
//    float weights_2 = boneWeight.z;
//    float weights_3 = boneWeight.w;
//    float4x4 bone_0 = float4x4(u_bones[index_0 * 3], u_bones[index_0 * 3 + 1], u_bones[index_0 * 3 + 2], vec4(0, 0, 0, 1));
//    float4x4 bone_1 = float4x4(u_bones[index_1 * 3], u_bones[index_1 * 3 + 1], u_bones[index_1 * 3 + 2], vec4(0, 0, 0, 1));
//    float4x4 bone_2 = float4x4(u_bones[index_2 * 3], u_bones[index_2 * 3 + 1], u_bones[index_2 * 3 + 2], vec4(0, 0, 0, 1));
//    float4x4 bone_3 = float4x4(u_bones[index_3 * 3], u_bones[index_3 * 3 + 1], u_bones[index_3 * 3 + 2], vec4(0, 0, 0, 1));
//    return bone_0 * weights_0 + bone_1 * weights_1 + bone_2 * weights_2 + bone_3 * weights_3;
//}
//#endif

v2f vert(appdata a)
{
    v2f o = (v2f)0;
    // o.pos = mul(uProjectionMatrix, mul(uViewMatrix, mul(uWorldMatrix, float4(a.inputPosition.xyz, 1.0))));
#if (SKIN_ON == 1)
    int index_0 = int(a.boneIndices.x);
    int index_1 = int(a.boneIndices.y);
    int index_2 = int(a.boneIndices.z);
    int index_3 = int(a.boneIndices.w);
    float weights_0 = a.boneWeight.x;
    float weights_1 = a.boneWeight.y;
    float weights_2 = a.boneWeight.z;
    float weights_3 = a.boneWeight.w;
    float4x4 bone_0 = float4x4(u_bones[index_0 * 3], u_bones[index_0 * 3 + 1], u_bones[index_0 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_1 = float4x4(u_bones[index_1 * 3], u_bones[index_1 * 3 + 1], u_bones[index_1 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_2 = float4x4(u_bones[index_2 * 3], u_bones[index_2 * 3 + 1], u_bones[index_2 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_3 = float4x4(u_bones[index_3 * 3], u_bones[index_3 * 3 + 1], u_bones[index_3 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 model_matrix = transpose(bone_0 * weights_0 + bone_1 * weights_1 + bone_2 * weights_2 + bone_3 * weights_3);
#else
    float4x4 model_matrix = u_model_matrix;
#endif
    o.vertex = mul(mul(mul(float4(a.vertex.xyz, 1.0), model_matrix), u_view_matrix), u_projection_matrix);
    o.vcolor = a.color;
    o.uv = a.uv;
#if (COMPILER_HLSL == 1)
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
#if (COMPILER_VULKAN == 1)
    o.vertex.y = -o.vertex.y;
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
    return o;
}