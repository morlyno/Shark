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
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;
 
//layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_UV;
layout(location = 2) flat out int v_ID;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    //v_Normal = a_Normal;
    v_UV = a_UV;
    v_ID = u_MeshData.ID;
}

#version 450 core
#pragma stage : Pixel

layout(binding = 0) uniform sampler2D u_Albedo;

//layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_UV;
layout(location = 2) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
    //o_Color = vec4(v_Normal, 1.0f);
    vec4 albedo = texture(u_Albedo, v_UV);
    if (albedo.w <= 0.0f)
        discard;

    o_Color = vec4(albedo.xyz, 1.0f);
    //o_Color = albedo;
    o_ID = v_ID;
}
