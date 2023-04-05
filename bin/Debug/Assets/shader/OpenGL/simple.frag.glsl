#version 420

struct simple_vert_output
{
    vec4 pos;
    vec2 uv;
};

uniform sampler2D SPIRV_Cross_Combinedtexsamp0;

layout(location = 0) in vec2 _entryPointOutput_uv;
layout(location = 0) out vec4 _entryPointOutput;

vec4 _simple_frag_main(simple_vert_output _entryPointOutput_1)
{
    return texture(SPIRV_Cross_Combinedtexsamp0, _entryPointOutput_1.uv);
}

void main()
{
    simple_vert_output _entryPointOutput_1;
    _entryPointOutput_1.pos = gl_FragCoord;
    _entryPointOutput_1.uv = _entryPointOutput_uv;
    simple_vert_output param = _entryPointOutput_1;
    _entryPointOutput = _simple_frag_main(param);
}

