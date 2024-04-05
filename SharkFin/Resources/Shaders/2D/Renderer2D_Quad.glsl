#version 450 core
#pragma stage : vertex

layout(set=1, binding=0) uniform Camera
{
    mat4 ViewProjection; 
} u_Camera;

struct VertexInput
{
    vec3 Position;
    vec4 Color;
    vec2 TexCoord;
    int TexIndex;
    float TilingFactor;
    int ID;
};

struct VertexOutput
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout(location=0) in vec3 a_Position;
layout(location=1) in vec4 a_Color;
layout(location=2) in vec2 a_TexCoord;
layout(location=3) in int a_TexIndex;
layout(location=4) in float a_TilingFactor;
layout(location=5) in int a_ID;

layout(location=0) out VertexOutput Output;
layout(location=3) out flat int TexIndex;
layout(location=4) out flat int ID;

void main()
{
    gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0f);
    Output.Color = a_Color;
    Output.TexCoord = a_TexCoord;
    Output.TilingFactor = a_TilingFactor;
    TexIndex = a_TexIndex;
    ID = a_ID;
}

#version 450 core
#pragma stage : Pixel

layout(set=0, binding=0) uniform sampler2D u_Textures[16];

struct PixelInput
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout(location=0) in PixelInput Input;
layout(location=3) in flat int TexIndex;
layout(location=4) in flat int ID;

layout(location=0) out vec4 o_Color;
layout(location=1) out int o_ID;

void main()
{
    vec3 color = vec3(0.0f);
    switch (TexIndex)
    {
        case  0: color = texture(u_Textures[ 0], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  1: color = texture(u_Textures[ 1], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  2: color = texture(u_Textures[ 2], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  3: color = texture(u_Textures[ 3], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  4: color = texture(u_Textures[ 4], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  5: color = texture(u_Textures[ 5], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  6: color = texture(u_Textures[ 6], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  7: color = texture(u_Textures[ 7], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  8: color = texture(u_Textures[ 8], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case  9: color = texture(u_Textures[ 9], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 10: color = texture(u_Textures[10], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 11: color = texture(u_Textures[11], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 12: color = texture(u_Textures[12], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 13: color = texture(u_Textures[13], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 14: color = texture(u_Textures[14], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
        case 15: color = texture(u_Textures[15], Input.TexCoord * Input.TilingFactor).rgb * Input.Color.rgb; break;
    }

    o_Color = vec4(color, 1.0f);
    o_ID = ID;
}