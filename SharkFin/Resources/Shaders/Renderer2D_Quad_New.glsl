#version 450 core
#pragma stage : vertex

layout(std140, binding = 0) uniform SceneData
{
    mat4 ViewProjection;
} u_SceneData;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int a_TexIndex;
layout(location = 4) in float a_TilingFactor;
layout(location = 5) in int a_ID;

struct VertexOutput
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout(location = 0) out VertexOutput Output;
layout(location = 3) out flat int v_TexIndex;
layout(location = 4) out flat int v_ID;

void main()
{
    Output.Color = a_Color;
    Output.TexCoord = a_TexCoord;
    Output.TilingFactor = a_TilingFactor;
    v_TexIndex = a_TexIndex;
    v_ID = a_ID;

    gl_Position = u_SceneData.ViewProjection * vec4(a_Position, 1.0f);
}


#version 450 core
#pragma stage : pixel

layout(binding = 0) uniform sampler2D u_Textures[16];

struct VertexOutput
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};


layout(location = 0) in VertexOutput Input;
layout(location = 3) in flat int v_TexIndex;
layout(location = 4) in flat int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
    switch (v_TexIndex)
    {
        case  0: o_Color = texture(u_Textures[ 0], Input.TexCoord * Input.TilingFactor); break;
        case  1: o_Color = texture(u_Textures[ 1], Input.TexCoord * Input.TilingFactor); break;
        case  2: o_Color = texture(u_Textures[ 2], Input.TexCoord * Input.TilingFactor); break;
        case  3: o_Color = texture(u_Textures[ 3], Input.TexCoord * Input.TilingFactor); break;
        case  4: o_Color = texture(u_Textures[ 4], Input.TexCoord * Input.TilingFactor); break;
        case  5: o_Color = texture(u_Textures[ 5], Input.TexCoord * Input.TilingFactor); break;
        case  6: o_Color = texture(u_Textures[ 6], Input.TexCoord * Input.TilingFactor); break;
        case  7: o_Color = texture(u_Textures[ 7], Input.TexCoord * Input.TilingFactor); break;
        case  8: o_Color = texture(u_Textures[ 8], Input.TexCoord * Input.TilingFactor); break;
        case  9: o_Color = texture(u_Textures[ 9], Input.TexCoord * Input.TilingFactor); break;
        case 10: o_Color = texture(u_Textures[10], Input.TexCoord * Input.TilingFactor); break;
        case 11: o_Color = texture(u_Textures[11], Input.TexCoord * Input.TilingFactor); break;
        case 12: o_Color = texture(u_Textures[12], Input.TexCoord * Input.TilingFactor); break;
        case 13: o_Color = texture(u_Textures[13], Input.TexCoord * Input.TilingFactor); break;
        case 14: o_Color = texture(u_Textures[14], Input.TexCoord * Input.TilingFactor); break;
        case 15: o_Color = texture(u_Textures[15], Input.TexCoord * Input.TilingFactor); break;
    }
    
    o_Color += Input.Color;
    o_ID = v_ID;
}