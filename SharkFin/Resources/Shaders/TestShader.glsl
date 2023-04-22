#version 450 core
#pragma stage : vertex

layout(std140, binding = 0) uniform SceneData
{
    mat4 ViewProjection;
} u_SceneData;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 v_TexCoord;

void main()
{
    gl_Position = u_SceneData.ViewProjection * vec4(a_Position, 1.0f);
}

#version 450 core
#pragma stage : pixel

layout(binding = 0) uniform sampler2D u_Texture_Set_0_Binding_0;
layout(binding = 1) uniform sampler2D u_Texture_Set_1_Binding_0;

layout(std430, binding = 2) readonly buffer ColorSSBO {
    vec3 Color;
};

layout(location = 0) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec4 col0 = texture(u_Texture_Set_0_Binding_0, v_TexCoord);
    vec4 col1 = texture(u_Texture_Set_1_Binding_0, v_TexCoord);

    o_Color = col0 * col1;
    o_Color *= vec4(Color, 1.0f);
}
