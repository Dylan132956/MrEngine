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

struct v2f
{
    float4 vertex : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 vfnormal : TEXCOORD1;
    float4 vftangent : TEXCOORD2;
    float4 vfbinormal : TEXCOORD3;
};