#version 450 core
#pragma stage : vertex

// begin_metadata
// set u_SceneData PerMaterial
// end_metadata

layout(std140, binding = 0) uniform SceneData
{
    mat4 SkyboxProjection;
} u_SceneData;

layout(location = 0) in vec3 a_Position;
layout(location = 0) out vec3 v_LocalPosition;

void main()
{
    v_LocalPosition = a_Position;
    gl_Position = (u_SceneData.SkyboxProjection * vec4(a_Position, 1.0)).xyww;
}

#version 450 core
#pragma stage : pixel

layout(binding = 0) uniform samplerCube u_EnvironmentMap;

layout(location = 0) in vec3 v_LocalPosition;
layout(location = 0) out vec4 o_Color;

void main()
{
    vec3 uv = normalize(v_LocalPosition);
    o_Color = textureLod(u_EnvironmentMap, uv, 0);
}
