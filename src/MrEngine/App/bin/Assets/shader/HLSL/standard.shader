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
#include "pbr.inc"

cbuffer PerView : register(b0)
{
    float4x4 u_view_matrix;
    float4x4 u_projection_matrix;
    float4 u_camera_pos;
    float4 u_time;
}

cbuffer PerRenderer : register(b1)
{
    float4x4 u_model_matrix;
}

#if (SKIN_ON == 1)
cbuffer PerRendererBones : register(b2)
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
    float4 boneWeight : BLENDWEIGHT;
    float4 boneIndices : BLENDINDICES;
};

cbuffer PerLightFragment : register(b6)
{
    float4 u_ambient_color;
    float4 u_light_pos;
    float4 u_light_color;
    float4 u_light_atten;
    float4 u_spot_light_dir;
    float4 u_shadow_params;
};

v2f vert(appdata v)
{
    v2f o = (v2f)0;
#if (SKIN_ON == 1)
    int index_0 = int(v.boneIndices.x);
    int index_1 = int(v.boneIndices.y);
    int index_2 = int(v.boneIndices.z);
    int index_3 = int(v.boneIndices.w);
    float weights_0 = v.boneWeight.x;
    float weights_1 = v.boneWeight.y;
    float weights_2 = v.boneWeight.z;
    float weights_3 = v.boneWeight.w;
    float4x4 bone_0 = float4x4(u_bones[index_0 * 3], u_bones[index_0 * 3 + 1], u_bones[index_0 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_1 = float4x4(u_bones[index_1 * 3], u_bones[index_1 * 3 + 1], u_bones[index_1 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_2 = float4x4(u_bones[index_2 * 3], u_bones[index_2 * 3 + 1], u_bones[index_2 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 bone_3 = float4x4(u_bones[index_3 * 3], u_bones[index_3 * 3 + 1], u_bones[index_3 * 3 + 2], float4(0, 0, 0, 1));
    float4x4 model_matrix = transpose(bone_0 * weights_0 + bone_1 * weights_1 + bone_2 * weights_2 + bone_3 * weights_3);
#else
    float4x4 model_matrix = u_model_matrix;
#endif
    float4 worldpos = mul(float4(v.vertex.xyz, 1.0), model_matrix);
    o.vertex = mul(mul(worldpos, u_view_matrix), u_projection_matrix);
    o.vfnormal.xyz = mul(v.normal.xyz, (float3x3)model_matrix);
#if (_NORMALMAP_ON == 1)
    o.vftangent.xyz = mul(v.tangent.xyz, (float3x3)model_matrix);
    o.vfbinormal.xyz = normalize(cross(o.vfnormal.xyz, o.vftangent.xyz) * float3(v.tangent.w, v.tangent.w, v.tangent.w));
#endif
    o.vfnormal.w = worldpos.x;
    o.vftangent.w = worldpos.y;
    o.vfbinormal.w = worldpos.z;
    o.vcolor = v.color;
    o.uv = v.uv;
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
    float2 uv = _entryPointOutput.uv;
    float4 bgfx_VoidFrag = vec4_splat(1.0);
    PBRMaterial mat = pbrMaterial(uv);
#if (_NORMALMAP_ON == 1)
    float3 N = convertTangentNormal(_entryPointOutput.vfnormal.xyz, _entryPointOutput.vftangent.xyz, mat.normal);
#else
    float3 N = _entryPointOutput.vfnormal.xyz;
#endif
    N.xyz = normalize(N.xyz);
    mat.a = specularAntiAliasing(N, mat.a);
    float3 camPos = u_camera_pos.xyz;
    float3 fragPos = float3(_entryPointOutput.vfnormal.w, _entryPointOutput.vftangent.w, _entryPointOutput.vfbinormal.w);
    float3 V = normalize(camPos - fragPos);
    float NoV = abs(dot(N, V)) + 1e-5;
    float3 msFactor = 1.0;
    float3 radianceOut = vec3_splat(0.0);
    float3 L = normalize(u_light_pos.xyz - fragPos * u_light_pos.w);
    float NoL = saturate(dot(N, L));
    float3 color = u_light_color.rgb * mat.occlusion;
    radianceOut += BRDF(V, L, N, NoV, NoL, mat) * color * msFactor * NoL;
    radianceOut += u_ambient_color.rgb * mat.diffuseColor * mat.occlusion;
    radianceOut += mat.emissive;
    outColor = float4(toGammaAccurate(tonemap_aces_luminance(radianceOut)), 1.0);
}
ENDCG
#END_PASSES