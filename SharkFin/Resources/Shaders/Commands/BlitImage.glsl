#version 450 core
#pragma stage : vertex

layout(location=0) in vec2 a_Position;
layout(location=0) out vec2 TexCoord;

void main()
{
    gl_Position = vec4(a_Position, 0.0f, 1.0f);
    TexCoord = vec2((a_Position.x + 1.0f) * 0.5f, 1.0f - ((a_Position.y + 1.0f) * 0.5f));
}

#version 450 core
#pragma stage : pixel

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 o_Result;
layout(set=1, binding=0) uniform sampler2D u_SourceImage;

void main()
{
     o_Result = texture(u_SourceImage, TexCoord);
}
