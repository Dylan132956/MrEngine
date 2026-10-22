#DEF_PASSES Skybox
#DEF_PARAMS
rs = {
    Cull = Off,
    ZTest = Always,
    ZWrite = Off,
    SrcBlendMode = One,
    DstBlendMode = Zero,
    CWrite = On,
    Queue = Background,
}
#END_PARAMS
CGPROGRAM
#include "vso.h.hlsl"

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

TextureCube g_SkyboxMap : reg(t0, space1);
SamplerState g_SkyboxSampler : reg(s0, space1);

struct appdata
{
    float4 vertex : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct sky_v2f
{
    float4 vertex : SV_POSITION;
    float3 uvw : TEXCOORD0;
};

float3 toGammaAccurate(float3 _rgb)
{
    float3 lo = _rgb * 12.92;
    float3 hi = pow(abs(_rgb), float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) * 1.055 - 0.055;
    float3 sel = float3(_rgb.x <= 0.0031308 ? 1 : 0, _rgb.y <= 0.0031308 ? 1 : 0, _rgb.z <= 0.0031308 ? 1 : 0);
    return lerp(hi, lo, sel);
}

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

float3 tonemap_uncharted2(float3 color)
{
    // Match DiligentFX ToneMapping.fxh Uncharted2 (ExposureBias=2).
    const float MiddleGray = 0.18;
    const float AverageLogLum = 0.30;
    const float WhitePoint = 3.0;
    const float ExposureBias = 2.0;
    float lumScale = MiddleGray / max(AverageLogLum, 1e-4);
    float3 scaled = color * lumScale;
    float3 curr = Uncharted2Tonemap(scaled * ExposureBias);
    float3 whiteScale = float3(1.0, 1.0, 1.0) / Uncharted2Tonemap(float3(WhitePoint, WhitePoint, WhitePoint));
    return saturate(curr * whiteScale);
}

sky_v2f vert(appdata v)
{
    sky_v2f o = (sky_v2f)0;
    float4x4 viewNoTranslate = u_view_matrix;
    viewNoTranslate[3][0] = 0;
    viewNoTranslate[3][1] = 0;
    viewNoTranslate[3][2] = 0;
    float4 pos = mul(mul(float4(v.vertex.xyz, 1.0), viewNoTranslate), u_projection_matrix);
    o.vertex = pos.xyww;
    o.uvw = v.vertex.xyz;
#if (COMPILER_HLSL == 1)
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
#if (COMPILER_VULKAN == 1)
    o.vertex.y = -o.vertex.y;
    o.vertex.z = 0.5 * (o.vertex.z + o.vertex.w);
#endif
    return o;
}

void frag(in sky_v2f i, out float4 outColor: SV_Target0)
{
    // DiligentFX GLTFViewer default background: prefiltered env map, mip 1.
    float3 color = g_SkyboxMap.SampleLevel(g_SkyboxSampler, i.uvw, 1.0).rgb;
    outColor = float4(toGammaAccurate(tonemap_uncharted2(color)), 1.0);
}
ENDCG
#END_PASSES
