#version 450 core
#pragma stage : vertex

layout(set = 1, binding = 0, std140) uniform Uniforms
{
    mat4 SkyboxProjection;
} u_Uniforms;

layout(location = 0) in vec3 a_Position;
layout(location = 0) out vec3 v_LocalPosition;

void main()
{
    v_LocalPosition = a_Position;
    gl_Position = (u_Uniforms.SkyboxProjection * vec4(a_Position, 1.0)).xyww;
}

#version 450 core
#pragma stage : pixel

layout(set = 1, binding = 2) uniform samplerCube u_EnvironmentMap;

layout(set = 1, binding = 1) uniform Settings
{
    float Lod;
    float Intensity;
    float P0, P1;
} u_Settings;

layout(location = 0) in vec3 v_LocalPosition;
layout(location = 0) out vec4 o_Color;

void main()
{
    vec3 uv = normalize(v_LocalPosition);
    vec3 color = textureLod(u_EnvironmentMap, uv, u_Settings.Lod).rgb * u_Settings.Intensity;
    o_Color = vec4(color, 1.0f);
}
