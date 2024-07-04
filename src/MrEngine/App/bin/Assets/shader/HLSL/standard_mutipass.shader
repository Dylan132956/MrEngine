#DEF_PASSES ForwardBase0
#DEF_PARAMS
rs = {
    Cull = Off,
    ZTest = LEqual,
    ZWrite = On,
    SrcBlendMode = One,
    DstBlendMode = Zero,
    CWrite = On,
    Queue = Geometry,
}
#END_PARAMS
CGPROGRAM
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

cbuffer cbGLTFAttribs : register(b4)
{
    GLTFMaterialShaderInfo g_MaterialInfo;
}

Texture2D g_ColorMap : reg(t0, space1);
SamplerState g_ColorSampler : reg(s0, space1);

Texture2D g_PhysicalDescriptorMap : reg(t1, space1);
SamplerState g_PhysicalDescriptorSampler : reg(s1, space1);

Texture2D g_NormalMap : reg(t2, space1);
SamplerState g_NormalSampler : reg(s2, space1);

Texture2D g_AOMap : reg(t3, space1);
SamplerState g_AOSampler : reg(s3, space1);

Texture2D g_EmissiveMap : reg(t4, space1);
SamplerState g_EmissiveSampler : reg(s4, space1);

v2f vert(appdata a)
{
    v2f o = (v2f)0;
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

void frag(in v2f _entryPointOutput, out float4 outColor: SV_Target0)
{
    outColor = g_MaterialInfo.BaseColorFactor * g_ColorMap.Sample(g_ColorSampler, _entryPointOutput.uv) * g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, _entryPointOutput.uv) * g_NormalMap.Sample(g_NormalSampler, _entryPointOutput.uv)
        * g_AOMap.Sample(g_AOSampler, _entryPointOutput.uv) + g_EmissiveMap.Sample(g_EmissiveSampler, _entryPointOutput.uv);
}
ENDCG
#END_PASSES

#DEF_PASSES ForwardBase1
#DEF_PARAMS
rs = {
    Cull = Off,
    ZTest = LEqual,
    ZWrite = On,
    SrcBlendMode = One,
    DstBlendMode = Zero,
    CWrite = On,
    Queue = Geometry,
}
#END_PARAMS
CGPROGRAM
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

cbuffer cbGLTFAttribs : register(b4)
{
    GLTFMaterialShaderInfo g_MaterialInfo;
}

Texture2D g_ColorMap : reg(t0, space1);
SamplerState g_ColorSampler : reg(s0, space1);

Texture2D g_PhysicalDescriptorMap : reg(t1, space1);
SamplerState g_PhysicalDescriptorSampler : reg(s1, space1);

Texture2D g_NormalMap : reg(t2, space1);
SamplerState g_NormalSampler : reg(s2, space1);

Texture2D g_AOMap : reg(t3, space1);
SamplerState g_AOSampler : reg(s3, space1);

Texture2D g_EmissiveMap : reg(t4, space1);
SamplerState g_EmissiveSampler : reg(s4, space1);

v2f vert(appdata a)
{
    v2f o = (v2f)0;
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

void frag(in v2f _entryPointOutput, out float4 outColor: SV_Target0)
{
    outColor = g_MaterialInfo.BaseColorFactor * g_ColorMap.Sample(g_ColorSampler, _entryPointOutput.uv) * g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, _entryPointOutput.uv) * g_NormalMap.Sample(g_NormalSampler, _entryPointOutput.uv)
        * g_AOMap.Sample(g_AOSampler, _entryPointOutput.uv) + g_EmissiveMap.Sample(g_EmissiveSampler, _entryPointOutput.uv);
}
ENDCG
#END_PASSES