#include "vso.h.hlsl"
struct a2v_simple {
    float4 inputPosition : TEXCOORD0;
};
simple_vert_output vert(a2v_simple a)
{
    simple_vert_output o;
    o.pos = float4(a.inputPosition.xyz, 1.0);
    return o;
}