#include "vso.h.hlsl"
struct SPIRV_Cross_Output1
{
    float4 o_color : SV_Target0;
};
SPIRV_Cross_Output1 frag(simple_vert_color_output _entryPointOutput)
{
    SPIRV_Cross_Output1 stage_output;
    stage_output.o_color = _entryPointOutput.v_color;
    return stage_output;
}