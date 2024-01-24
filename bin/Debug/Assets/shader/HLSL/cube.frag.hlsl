#include "vso.h.hlsl"

Texture2D tex0 : reg(t0, space1);
SamplerState samp0 : reg(s0, space1);

struct SPIRV_Cross_Output
{
    float4 o_color : SV_Target0;
};
SPIRV_Cross_Output frag(simple_vert_color_uv_output _entryPointOutput)
{
    SPIRV_Cross_Output stage_output;
    stage_output.o_color = _entryPointOutput.v_color * tex0.Sample(samp0, _entryPointOutput.v_uv);
    //stage_output.o_color = float4(_entryPointOutput.v_uv, 0.0, 1.0);
    return stage_output;
}