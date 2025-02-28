#if (COMPILER_VULKAN == 1)
#define reg(a, b) register(a, b)
#else
#define reg(a, b) register(a)
#endif

struct simple_vert_output
{
    float4 pos : SV_Position;
};

struct simple_vert_color_output
{
    float4 pos : SV_Position;
    float4 v_color : COLOR;
};

struct simple_vert_color_uv_output
{
    float4 pos : SV_Position;
    float4 v_color : COLOR;
    float2 v_uv : TEXCOORD;
};

struct v2f
{
    float4 vertex : SV_POSITION;
    float4 color  : COLOR;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float4 binormal : BINORMAL;
    float4 u_camera_pos : TEXCOORD2;
};
