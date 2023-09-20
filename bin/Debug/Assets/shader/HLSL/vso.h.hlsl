struct simple_vert_output
{
    float4 pos : SV_Position;
};

struct simple_vert_color_output
{
    float4 pos : SV_Position;
    float4 v_color : TEXCOORD0;
};