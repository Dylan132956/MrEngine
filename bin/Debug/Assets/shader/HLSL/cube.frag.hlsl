#include "vso.h.hlsl"
struct SPIRV_Cross_Output
{
    float4 o_color : SV_Target0;
};
SPIRV_Cross_Output frag(simple_vert_output _entryPointOutput)
{
    SPIRV_Cross_Output stage_output;
    stage_output.o_color = float4(1.0, 1.0, 0.0, 1.0);
    return stage_output;
}