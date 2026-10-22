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
    LightMode = Forward,
}
#END_PARAMS
CGPROGRAM
#include "vso.h.hlsl"
#include "pbr.inc"

cbuffer PerView : register(b1)
{
    float4x4 u_view_matrix;
    float4x4 u_projection_matrix;
    float4 u_camera_pos;
    float4 u_time;
}

cbuffer PerRenderer : register(b2)
{
    float4x4 u_model_matrix;
}

#if (SKIN_ON == 1)
cbuffer PerRendererBones : register(b3)
{
    float4 u_bones[210];
};
#endif

float4 MR_color;

struct appdata
{
    float4 vertex : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 boneWeights : BLENDWEIGHT;
    float4 boneIndices : BLENDINDICES;
};

cbuffer PerLightFragment : register(b7)
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
    float weights_0 = v.boneWeights.x;
    float weights_1 = v.boneWeights.y;
    float weights_2 = v.boneWeights.z;
    float weights_3 = v.boneWeights.w;
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
    o.normal.xyz = mul(v.normal.xyz, (float3x3)model_matrix);
    o.tangent.xyz = mul(v.tangent.xyz, (float3x3)model_matrix);
    o.binormal.xyz = cross(o.normal.xyz, o.tangent.xyz) * v.tangent.w;
    o.normal.w = worldpos.x;
    o.tangent.w = worldpos.y;
    o.binormal.w = worldpos.z;
    o.color = v.color * MR_color;
    o.u_camera_pos = u_camera_pos;
    o.uv = v.uv;
    o.uv2 = v.uv2;
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
    PBRMaterial mat = pbrMaterial(uv);

    float3 camPos = _entryPointOutput.u_camera_pos.xyz;
    float3 fragPos = float3(_entryPointOutput.normal.w, _entryPointOutput.tangent.w, _entryPointOutput.binormal.w);

    float3 Ng = normalize(_entryPointOutput.normal.xyz);
    float3 T = _entryPointOutput.tangent.xyz;
    float3 B = _entryPointOutput.binormal.xyz;
    // Meshes without TANGENT: derive the cotangent frame from screen-space derivatives.
    float3 dp1 = ddx(fragPos);
    float3 dp2 = ddy(fragPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);
    if (dot(T, T) < 1e-8 || dot(B, B) < 1e-8)
    {
        float3 dp2perp = cross(dp2, Ng);
        float3 dp1perp = cross(Ng, dp1);
        T = dp2perp * duv1.x + dp1perp * duv2.x;
        B = dp2perp * duv1.y + dp1perp * duv2.y;
        float maxLen = max(dot(T, T), dot(B, B));
        if (maxLen < 1e-20)
        {
            T = float3(1, 0, 0);
            B = float3(0, 1, 0);
        }
    }
    float3 N = applyTangentNormal(Ng, T, B, mat.normal);
    mat.a = specularAntiAliasing(N, mat.a);

    float3 V = normalize(camPos - fragPos);
    float NoV = abs(dot(N, V)) + 1e-5;

    float3 lightVec = u_light_pos.xyz - fragPos * u_light_pos.w;
    float3 L = normalize(lightVec);
    float NoL = saturate(dot(N, L));

    float atten = 1.0;
    if (u_light_pos.w > 0.0)
    {
        atten = GetDistanceAttenuation(lightVec, u_light_atten.z);
        // Spot light type == 1 in light_color.a
        if (u_light_color.a > 0.5 && u_light_color.a < 1.5)
        {
            atten *= GetSpotAngleAttenuation(L, u_spot_light_dir.xyz, u_light_atten.x, u_light_atten.y);
        }
    }

    float3 radianceOut = vec3_splat(0.0);
#if (LIGHT_ADD_ON == 1)
#else
    radianceOut += GetIBLContribution(mat, N, V, NoV);
    radianceOut += u_ambient_color.rgb * mat.diffuseColor * mat.occlusion;
    radianceOut += mat.emissive;
#endif
    radianceOut += BRDF(V, L, N, NoV, NoL, mat) * u_light_color.rgb * atten * NoL;

    float outAlpha = 1.0;
    if (g_MaterialInfo.AlphaMode == ALPHA_MODE_BLEND)
    {
        outAlpha = mat.alpha;
    }

    outColor = float4(toGammaAccurate(tonemap_aces_luminance(radianceOut)), outAlpha) * float4(_entryPointOutput.color.rgb, 1.0);
}
ENDCG
#END_PASSES
