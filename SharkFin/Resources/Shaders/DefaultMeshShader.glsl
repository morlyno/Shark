#version 450 core
#pragma stage : Vertex

layout(std140, binding = 0) uniform SceneData
{
    mat4 ViewProjection;
} u_SceneData;

layout(std140, binding = 1) uniform MeshData
{
    mat4 Transform;
    int ID;
    int dummy1;
    int dummy2;
    int dummy3;
} u_MeshData;

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out int v_ID;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    vec3 color = a_Position;
    color += vec3(1.0f, 1.0f, 1.0f);
    color *= 0.5f;
    v_Color = vec4(color, 1.0f);
    v_ID = u_MeshData.ID;

}

#version 450 core
#pragma stage : Pixel

layout(binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec4 v_Color;
layout(location = 1) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
    o_Color = v_Color;
    o_ID = v_ID;
}
