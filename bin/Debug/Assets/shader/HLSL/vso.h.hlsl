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
    float4 v_color : COLOR;
    float2 v_uv : TEXCOORD0;
};

struct GLTFMaterialShaderInfo
{
    float4  BaseColorFactor;
    float4  EmissiveFactor;
    float4  SpecularFactor;

    int     Workflow;
    float   BaseColorTextureUVSelector;
    float   PhysicalDescriptorTextureUVSelector;
    float   NormalTextureUVSelector;

    float   OcclusionTextureUVSelector;
    float   EmissiveTextureUVSelector;
    float   BaseColorSlice;
    float   PhysicalDescriptorSlice;

    float   NormalSlice;
    float   OcclusionSlice;
    float   EmissiveSlice;
    float   MetallicFactor;

    float   RoughnessFactor;
    int     AlphaMode;
    float   AlphaMaskCutoff;
    float   Dummy0;

    // When texture atlas is used, UV scale and bias applied to
    // each texture coordinate set
    float4 BaseColorUVScaleBias;
    float4 PhysicalDescriptorUVScaleBias;
    float4 NormalMapUVScaleBias;
    float4 OcclusionUVScaleBias;
    float4 EmissiveUVScaleBias;
};

struct v2f
{
    float4 vertex : SV_POSITION;
    float4 vcolor : COLOR;
    float2 uv : TEXCOORD0;
    float4 vfnormal : NORMAL;
    float4 vftangent : TANGENT;
};