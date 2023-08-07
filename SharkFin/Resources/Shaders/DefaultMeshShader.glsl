#version 450 core
#pragma stage : Vertex

#define BINDING_PER_SCENE 0
#define BINDING_PER_DRAWCALL 1

layout(std140, binding = BINDING_PER_SCENE) uniform SceneData
{
    mat4 ViewProjection;
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

layout(std140, binding = 3) uniform PBRData
{
    vec3 Albedo;
    float padding;
} u_PBRData;

//layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_UV;
layout(location = 2) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
    //o_Color = vec4(v_Normal, 1.0f);
    vec4 color = texture(u_Albedo, v_UV);
    color *= vec4(u_PBRData.Albedo, 1.0f);
    if (color.w <= 0.0f)
        discard;

    o_Color = vec4(color.xyz, 1.0f);
    o_ID = v_ID;
}
