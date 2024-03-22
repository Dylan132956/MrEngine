#include "vso.h.hlsl"

//Texture2D tex0 : register(t0);
//SamplerState samp0 : register(s0);
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

void frag(v2f _entryPointOutput, out float4 outColor: SV_Target0)
{
    outColor = g_MaterialInfo.BaseColorFactor * g_ColorMap.Sample(g_ColorSampler, _entryPointOutput.uv) * g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, _entryPointOutput.uv) * g_NormalMap.Sample(g_NormalSampler, _entryPointOutput.uv)
       * g_AOMap.Sample(g_AOSampler, _entryPointOutput.uv) + g_EmissiveMap.Sample(g_EmissiveSampler, _entryPointOutput.uv);
    //outColor = float4(1.0, 0.0, 0.0, 1.0);
    //outColor = g_PhysicalDescriptorMap.Sample(g_PhysicalDescriptorSampler, i.uv);
    //outColor = g_ColorMap.Sample(g_ColorSampler, i.uv);
    //outColor = float4(i.uv, 0.0, 1.0);
}