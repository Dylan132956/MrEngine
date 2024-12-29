struct PBRMaterial
{
    float4 albedo;
    float metallic;
    float roughness;
    float3 normal;
    float occlusion;
    float3 emissive;
    float3 diffuseColor;
    float3 F0;
    float a;
};

struct GLTFMaterialShaderInfo
{
    float4  BaseColorFactor;
    float4  EmissiveFactor;
    float4  SpecularFactor;

    int     Workflow;
    float   BaseColorTextureUVSelector;
    float   PhysicalDescriptorTextureUVSelector;
    float   NormalTextureUVSelector;

    float   OcclusionTextureUVSelector;
    float   EmissiveTextureUVSelector;
    float   BaseColorSlice;
    float   PhysicalDescriptorSlice;

    float   NormalSlice;
    float   OcclusionSlice;
    float   EmissiveSlice;
    float   MetallicFactor;

    float   RoughnessFactor;
    int     AlphaMode;
    float   AlphaMaskCutoff;
    float   Dummy0;

    // When texture atlas is used, UV scale and bias applied to
    // each texture coordinate set
    float4 BaseColorUVScaleBias;
    float4 PhysicalDescriptorUVScaleBias;
    float4 NormalMapUVScaleBias;
    float4 OcclusionUVScaleBias;
    float4 EmissiveUVScaleBias;
};

Texture2D g_ColorMap : reg(t0, space1);
SamplerState g_ColorSampler : reg(s0, space1);

Texture2D g_PhysicalDescriptorMap : reg(t1, space1);
SamplerState g_PhysicalDescriptorSampler : reg(s1, space1);

#if (_NORMALMAP_ON == 1)
Texture2D g_NormalMap : reg(t2, space1);
SamplerState g_NormalSampler : reg(s2, space1);
#endif

Texture2D g_AOMap : reg(t3, space1);
SamplerState g_AOSampler : reg(s3, space1);

Texture2D g_EmissiveMap : reg(t4, space1);
SamplerState g_EmissiveSampler : reg(s4, space1);

cbuffer PerMaterialFragment : register(b4)
{
    GLTFMaterialShaderInfo g_MaterialInfo;
}

float mix(float _a, float _b, float _t)
{
    return lerp(_a, _b, _t);
}

float2 mix(float2 _a, float2 _b, float2 _t)
{
    return lerp(_a, _b, _t);
}

float3 mix(float3 _a, float3 _b, float3 _t)
{
    return lerp(_a, _b, _t);
}

float4 mix(float4 _a, float4 _b, float4 _t)
{
    return lerp(_a, _b, _t);
}

bool2 lessThanEqual(float2 _a, float2 _b)
{
    return _a <= _b;
}

bool3 lessThanEqual(float3 _a, float3 _b)
{
    return _a <= _b;
}

bool4 lessThanEqual(float4 _a, float4 _b)
{
    return _a <= _b;
}

float3 vec3_splat(float _x)
{
    return float3(_x, _x, _x);
}

float4 vec4_splat(float _x)
{
    return float4(_x, _x, _x, _x);
}

float3 toLinearAccurate(float3 _rgb)
{
    float3 lo = _rgb / 12.92;
    float3 hi = pow((abs(_rgb) + 0.055) / 1.055, vec3_splat(2.4));
    float3 rgb = mix(hi, lo, float3(lessThanEqual(_rgb, vec3_splat(0.04045))));
    return rgb;
}

float4 toLinearAccurate(float4 _rgba)
{
    return float4(toLinearAccurate(_rgba.xyz), _rgba.w);
}

float3 toGammaAccurate(float3 _rgb)
{
    float3 lo = _rgb * 12.92;
    float3 hi = pow(abs(_rgb), vec3_splat(1.0 / 2.4)) * 1.055 - 0.055;
    float3 rgb = mix(hi, lo, float3(lessThanEqual(_rgb, vec3_splat(0.0031308))));
    return rgb;
}

float4 pbrBaseColor(float2 texcoord)
{
    return toLinearAccurate(g_ColorMap.Sample(g_ColorSampler, texcoord)) * g_MaterialInfo.BaseColorFactor;
}

float2 pbrMetallicRoughness(float2 texcoord)
{
    return g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, texcoord).rg * float2(g_MaterialInfo.MetallicFactor, g_MaterialInfo.RoughnessFactor);
}
#if (_NORMALMAP_ON == 1)
float3 pbrNormal(float2 texcoord)
{
    return normalize((g_NormalMap.Sample(g_NormalSampler, texcoord).rgb * 2.0) - 1.0);
}
#endif
float pbrOcclusion(float2 texcoord)
{
    float occlusion = g_AOMap.Sample(g_AOSampler, texcoord).r;
    return occlusion;
}

float3 pbrEmissive(float2 texcoord)
{
    return toLinearAccurate(g_EmissiveMap.Sample(g_EmissiveSampler, texcoord).rgb) * g_MaterialInfo.EmissiveFactor.rgb;
}

PBRMaterial pbrInitMaterial(PBRMaterial mat);
PBRMaterial pbrMaterial(float2 texcoord)
{
    PBRMaterial mat;
    mat.albedo = pbrBaseColor(texcoord);
    float2 metallicRoughness = pbrMetallicRoughness(texcoord);
    mat.metallic = metallicRoughness.r;
    mat.roughness = metallicRoughness.g;
#if (_NORMALMAP_ON == 1)
    mat.normal = pbrNormal(texcoord);
#endif
    mat.occlusion = pbrOcclusion(texcoord);
    mat.emissive = pbrEmissive(texcoord);
    mat = pbrInitMaterial(mat);
    return mat;
}

PBRMaterial pbrInitMaterial(PBRMaterial mat)
{
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    const float3 black = float3(0.0, 0.0, 0.0);
    mat.diffuseColor = mix(mat.albedo.rgb * (vec3_splat(1.0) - dielectricSpecular), black, mat.metallic);
    mat.F0 = mix(dielectricSpecular, mat.albedo.rgb, mat.metallic);
    mat.a = mat.roughness * mat.roughness;
    mat.a = max(mat.a, 0.01);
    return mat;
}

float specularAntiAliasing(float3 N, float a)
{
    const float SIGMA2 = 0.25;
    const float KAPPA = 0.18;
    float3 dndu = ddx(N);
    float3 dndv = ddy(-(N));
    float variance = SIGMA2 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, KAPPA);
    return saturate(a + kernelRoughness2);
}

float3 F_Schlick(float VoH, float3 F0)
{
    float f = pow(1.0 - VoH, 5.0);
    return f + F0 * (1.0 - f);
}

float D_GGX(float NoH, float a)
{
    a = NoH * a;
    float k = a / (1.0 - NoH * NoH + a * a);
    return k * k * (0.31830988618);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGX(float NoV, float NoL, float a)
{
    float a2 = a * a;
    float GGXV = NoV + sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoL + sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 1.0 / (GGXV * GGXL);
}

float Fd_Lambert()
{
    return (0.31830988618);
}

float3 BRDF(float3 v, float3 l, float3 n, float NoV, float NoL, PBRMaterial mat)
{
    float3 h = normalize(l + v);
    float NoH = saturate(dot(n, h));
    float VoH = saturate(dot(v, h));
    float D = D_GGX(NoH, mat.a);
    float3 F = F_Schlick(VoH, mat.F0);
    float V = V_SmithGGXCorrelated(NoV, NoL, mat.a);
    float3 Fr = F * (V * D);
    float3 Fd = mat.diffuseColor * Fd_Lambert();
    return Fr + (1.0 - F) * Fd;
}

float3x3 mtxFromCols(float3 _0, float3 _1, float3 _2)
{
    return transpose(float3x3(_0, _1, _2));
}

float4x4 mtxFromCols(float4 _0, float4 _1, float4 _2, float4 _3)
{
    return transpose(float4x4(_0, _1, _2, _3));
}

float3 convertTangentNormal(float3 normal_ref, float3 tangent_ref, float3 normal)
{
    float3 bitangent = cross(normal_ref, tangent_ref);
    float3x3 TBN = mtxFromCols(
        normalize(tangent_ref),
        normalize(bitangent),
        normalize(normal_ref)
    );
    return normalize(mul(TBN, normal));
}

float3 tonemap_aces_luminance(float3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    float3 x = color * 0.6;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}
