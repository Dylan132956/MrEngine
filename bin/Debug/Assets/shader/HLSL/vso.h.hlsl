struct simple_vert_output
{
    float4 pos : SV_Position;
};

struct simple_vert_color_output
{
    float4 pos : SV_Position;
    float4 v_color : TEXCOORD0;
};

struct simple_vert_color_uv_output
{
    float4 pos : SV_Position;
    float4 v_color : TEXCOORD0;
    float2 v_uv : TEXCOORD1;
};