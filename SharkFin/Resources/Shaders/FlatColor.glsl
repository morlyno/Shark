#version 450 core
#pragma stage : Vertex

// begin_metadata
// set binding=1 UpdateFrequency::PerMaterial
// end_metadata

#define BINDING_PER_SCENE 0
#define BINDING_PER_DRAWCALL 1

layout(std140, binding = BINDING_PER_SCENE) uniform SceneData
{
    mat4 ViewProjection;
    vec3 CameraPosition;
    float Padding;
} u_SceneData;

layout(std140, binding = BINDING_PER_DRAWCALL) uniform MeshData
{
    mat4 Transform;
    int ID;
    int padding1;
    int padding2;
    int padding3;
} u_MeshData;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;
 
layout(location = 0) flat out int v_ID;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    v_ID = u_MeshData.ID;
}

#version 450 core
#pragma stage : Pixel

// begin_metadata
// set binding=0 UpdateFrequency::PerMaterial
// end_metadata

layout(std140, binding = 0) uniform Mesh
{
    vec4 Color;
} u_Mesh;

layout(location = 0) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
    if (u_Mesh.Color.w <= 0.0f)
        discard;

    o_Color = vec4(u_Mesh.Color.xyz, 1.0f);
    o_ID = v_ID;
}
