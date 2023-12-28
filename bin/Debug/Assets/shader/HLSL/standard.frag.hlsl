#include "vso.h.hlsl"

//Texture2D tex0 : register(t0);
//SamplerState samp0 : register(s0);
cbuffer cbGLTFAttribs : register(b4)
{
    GLTFMaterialShaderInfo g_MaterialInfo;
}


Texture2D g_ColorMap : register(t0);
SamplerState g_ColorSampler : register(s0);

Texture2D g_PhysicalDescriptorMap : register(t1);
SamplerState g_PhysicalDescriptorSampler : register(s1);

Texture2D g_NormalMap : register(t2);
SamplerState g_NormalSampler : register(s2);

Texture2D g_AOMap : register(t3);
SamplerState g_AOSampler : register(s3);

Texture2D g_EmissiveMap : register(t4);
SamplerState g_EmissiveSampler : register(s4);

void frag(in v2f i, out float4 outColor: SV_Target0)
{
    outColor = g_MaterialInfo.BaseColorFactor * g_ColorMap.Sample(g_ColorSampler, i.uv) * g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, i.uv) * g_NormalMap.Sample(g_NormalSampler, i.uv)
       * g_AOMap.Sample(g_AOSampler, i.uv) + g_EmissiveMap.Sample(g_EmissiveSampler, i.uv);
    //outColor = float4(1.0, 0.0, 0.0, 1.0);
    //outColor = g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, i.uv);
}