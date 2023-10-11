#include "vso.h.hlsl"
struct SPIRV_Cross_Output
{
    float4 o_color : SV_Target0;
};
SPIRV_Cross_Output frag(simple_vert_color_uv_output _entryPointOutput)
{
    SPIRV_Cross_Output stage_output;
    stage_output.o_color = _entryPointOutput.v_color;
    return stage_output;
}