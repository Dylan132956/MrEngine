// DiligentFX / Khronos glTF PBR helpers. Layout must match Material::ShaderAttribs.

#ifndef PBR_WORKFLOW_METALL_ROUGH
#define PBR_WORKFLOW_METALL_ROUGH 0
#endif
#ifndef PBR_WORKFLOW_SPEC_GLOSS
#define PBR_WORKFLOW_SPEC_GLOSS 1
#endif
#ifndef ALPHA_MODE_OPAQUE
#define ALPHA_MODE_OPAQUE 0
#endif
#ifndef ALPHA_MODE_MASK
#define ALPHA_MODE_MASK 1
#endif
#ifndef ALPHA_MODE_BLEND
#define ALPHA_MODE_BLEND 2
#endif

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
    float alpha;
};

struct GLTFMaterialShaderInfo
{
    float4 BaseColorFactor;
    float4 EmissiveFactor;
    float4 SpecularFactor;

    int Workflow;
    float BaseColorTextureUVSelector;
    float PhysicalDescriptorTextureUVSelector;
    float NormalTextureUVSelector;

    float OcclusionTextureUVSelector;
    float EmissiveTextureUVSelector;
    float BaseColorSlice;
    float PhysicalDescriptorSlice;

    float NormalSlice;
    float OcclusionSlice;
    float EmissiveSlice;
    float MetallicFactor;

    float RoughnessFactor;
    int AlphaMode;
    float AlphaCutoff;
    float Dummy0;

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

Texture2D g_NormalMap : reg(t2, space1);
SamplerState g_NormalSampler : reg(s2, space1);

Texture2D g_AOMap : reg(t3, space1);
SamplerState g_AOSampler : reg(s3, space1);

Texture2D g_EmissiveMap : reg(t4, space1);
SamplerState g_EmissiveSampler : reg(s4, space1);

TextureCube g_IrradianceMap : reg(t5, space1);
SamplerState g_IrradianceSampler : reg(s5, space1);

TextureCube g_PrefilteredMap : reg(t6, space1);
SamplerState g_PrefilteredSampler : reg(s6, space1);

Texture2D g_BRDFLUT : reg(t7, space1);
SamplerState g_BRDFLUTSampler : reg(s7, space1);

cbuffer PerMaterialFragment : register(b5)
{
    GLTFMaterialShaderInfo g_MaterialInfo;
}

float mix(float _a, float _b, float _t) { return lerp(_a, _b, _t); }
float2 mix(float2 _a, float2 _b, float2 _t) { return lerp(_a, _b, _t); }
float3 mix(float3 _a, float3 _b, float3 _t) { return lerp(_a, _b, _t); }
float4 mix(float4 _a, float4 _b, float4 _t) { return lerp(_a, _b, _t); }

bool2 lessThanEqual(float2 _a, float2 _b) { return _a <= _b; }
bool3 lessThanEqual(float3 _a, float3 _b) { return _a <= _b; }
bool4 lessThanEqual(float4 _a, float4 _b) { return _a <= _b; }

float3 vec3_splat(float _x) { return float3(_x, _x, _x); }
float4 vec4_splat(float _x) { return float4(_x, _x, _x, _x); }

float3 toLinearAccurate(float3 _rgb)
{
    float3 lo = _rgb / 12.92;
    float3 hi = pow((abs(_rgb) + 0.055) / 1.055, vec3_splat(2.4));
    return mix(hi, lo, float3(lessThanEqual(_rgb, vec3_splat(0.04045))));
}

float4 toLinearAccurate(float4 _rgba)
{
    return float4(toLinearAccurate(_rgba.xyz), _rgba.w);
}

float3 toGammaAccurate(float3 _rgb)
{
    float3 lo = _rgb * 12.92;
    float3 hi = pow(abs(_rgb), vec3_splat(1.0 / 2.4)) * 1.055 - 0.055;
    return mix(hi, lo, float3(lessThanEqual(_rgb, vec3_splat(0.0031308))));
}

float2 ApplyUVScaleBias(float2 uv, float4 scaleBias)
{
    return uv * scaleBias.xy + scaleBias.zw;
}

float4 pbrBaseColor(float2 texcoord)
{
    float2 uv = ApplyUVScaleBias(texcoord, g_MaterialInfo.BaseColorUVScaleBias);
    return toLinearAccurate(g_ColorMap.Sample(g_ColorSampler, uv)) * g_MaterialInfo.BaseColorFactor;
}

float3 pbrNormalTS(float2 texcoord)
{
    float2 uv = ApplyUVScaleBias(texcoord, g_MaterialInfo.NormalMapUVScaleBias);
    float3 n = g_NormalMap.Sample(g_NormalSampler, uv).xyz * 2.0 - 1.0;
    return normalize(n);
}

float pbrOcclusion(float2 texcoord)
{
    float2 uv = ApplyUVScaleBias(texcoord, g_MaterialInfo.OcclusionUVScaleBias);
    return g_AOMap.Sample(g_AOSampler, uv).r;
}

float3 pbrEmissive(float2 texcoord)
{
    float2 uv = ApplyUVScaleBias(texcoord, g_MaterialInfo.EmissiveUVScaleBias);
    return toLinearAccurate(g_EmissiveMap.Sample(g_EmissiveSampler, uv).rgb) * g_MaterialInfo.EmissiveFactor.rgb;
}

PBRMaterial pbrInitMaterial(PBRMaterial mat);
PBRMaterial pbrMaterial(float2 texcoord)
{
    PBRMaterial mat = (PBRMaterial)0;
    mat.albedo = pbrBaseColor(texcoord);
    mat.alpha = mat.albedo.a;
    mat.normal = pbrNormalTS(texcoord);
    mat.occlusion = pbrOcclusion(texcoord);
    mat.emissive = pbrEmissive(texcoord);

    float2 physUV = ApplyUVScaleBias(texcoord, g_MaterialInfo.PhysicalDescriptorUVScaleBias);
    float4 physical = g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, physUV);

    if (g_MaterialInfo.Workflow == PBR_WORKFLOW_SPEC_GLOSS)
    {
        float3 specular = toLinearAccurate(physical.rgb) * g_MaterialInfo.SpecularFactor.rgb;
        float glossiness = physical.a * g_MaterialInfo.SpecularFactor.a;
        mat.metallic = 0.0;
        mat.roughness = saturate(1.0 - glossiness);
        mat.diffuseColor = mat.albedo.rgb * (1.0 - max(max(specular.r, specular.g), specular.b));
        mat.F0 = specular;
        mat.a = max(mat.roughness * mat.roughness, 0.01);
    }
    else
    {
        // glTF metallic-roughness: G = roughness, B = metallic
        mat.roughness = saturate(physical.g * g_MaterialInfo.RoughnessFactor);
        mat.metallic = saturate(physical.b * g_MaterialInfo.MetallicFactor);
        mat = pbrInitMaterial(mat);
    }

    if (g_MaterialInfo.AlphaMode == ALPHA_MODE_MASK)
    {
        clip(mat.alpha - g_MaterialInfo.AlphaCutoff);
    }

    return mat;
}

PBRMaterial pbrInitMaterial(PBRMaterial mat)
{
    const float3 dielectricSpecular = float3(0.04, 0.04, 0.04);
    const float3 black = float3(0.0, 0.0, 0.0);
    mat.diffuseColor = mix(mat.albedo.rgb * (vec3_splat(1.0) - dielectricSpecular), black, mat.metallic);
    mat.F0 = mix(dielectricSpecular, mat.albedo.rgb, mat.metallic);
    mat.a = max(mat.roughness * mat.roughness, 0.01);
    return mat;
}

float specularAntiAliasing(float3 N, float a)
{
    // Filament-style: filter in a^2 (alpha^2) domain, then convert back to a.
    const float SIGMA2 = 0.25;
    const float KAPPA = 0.18;
    float3 dndu = ddx(N);
    float3 dndv = ddy(-(N));
    float variance = SIGMA2 * (dot(dndu, dndu) + dot(dndv, dndv));
    float kernelRoughness2 = min(2.0 * variance, KAPPA);
    float a2 = a * a;
    return sqrt(saturate(a2 + kernelRoughness2));
}

float3 F_Schlick(float VoH, float3 F0)
{
    float f = pow(1.0 - VoH, 5.0);
    return f + F0 * (1.0 - f);
}

float3 F_SchlickRoughness(float NoV, float3 F0, float roughness)
{
    float f = pow(1.0 - NoV, 5.0);
    return F0 + (max(vec3_splat(1.0 - roughness), F0) - F0) * f;
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

float GetDistanceAttenuation(float3 lightVec, float invRangeSq)
{
    float distSq = max(dot(lightVec, lightVec), 1e-4);
    float factor = distSq * invRangeSq;
    float smoothFactor = saturate(1.0 - factor * factor);
    return (smoothFactor * smoothFactor) / distSq;
}

float GetSpotAngleAttenuation(float3 L, float3 spotDir, float cosOuter, float invCosDiff)
{
    float cd = dot(-L, spotDir);
    return saturate((cd - cosOuter) * invCosDiff);
}

float3x3 mtxFromCols(float3 _0, float3 _1, float3 _2)
{
    return transpose(float3x3(_0, _1, _2));
}

float3 applyTangentNormal(float3 N, float3 T, float3 B, float3 normalTS)
{
    float3x3 TBN = mtxFromCols(normalize(T), normalize(B), normalize(N));
    return normalize(mul(TBN, normalTS));
}

float3 GetIBLContribution(PBRMaterial mat, float3 N, float3 V, float NoV)
{
    float3 R = reflect(-V, N);
    float3 F = F_SchlickRoughness(NoV, mat.F0, mat.roughness);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - mat.metallic);

    // Linear HDR float cubemaps. Irradiance stores E/pi.
    float3 irradiance = g_IrradianceMap.Sample(g_IrradianceSampler, N).rgb;
    float3 diffuse = irradiance * mat.diffuseColor;

    const float MAX_REFLECTION_LOD = 6.0;
    float3 prefiltered = g_PrefilteredMap.SampleLevel(g_PrefilteredSampler, R, mat.roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = g_BRDFLUT.Sample(g_BRDFLUTSampler, float2(NoV, mat.roughness)).rg;
    float3 specular = prefiltered * (F * brdf.x + brdf.y);

    // DiligentFX GLTFViewer default.
    const float IBLScale = 1.0;
    return (kD * diffuse + specular) * mat.occlusion * IBLScale;
}

// DiligentFX ToneMapping.fxh Uncharted2 path (ExposureBias=2).
float3 Uncharted2Tonemap(float3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 tonemap_aces_luminance(float3 color)
{
    // DiligentFX GLTFViewer defaults
    const float MiddleGray = 0.18;
    const float AverageLogLum = 0.30;
    const float WhitePoint = 3.0;
    const float ExposureBias = 2.0;
    float lumScale = MiddleGray / max(AverageLogLum, 1e-4);
    float3 scaled = color * lumScale;
    float3 curr = Uncharted2Tonemap(scaled * ExposureBias);
    float3 whiteScale = vec3_splat(1.0) / Uncharted2Tonemap(vec3_splat(WhitePoint));
    return saturate(curr * whiteScale);
}
