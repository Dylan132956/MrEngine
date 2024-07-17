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
    float4 vcolor : COLOR;
    float2 uv : TEXCOORD0;
    float4 vfnormal : NORMAL;
    float4 vftangent : TANGENT;
    float4 vfbinormal : BINORMAL;
};