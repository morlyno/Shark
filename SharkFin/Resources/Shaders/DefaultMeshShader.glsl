#version 450 core
#pragma stage : Vertex

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
 
layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_UV;
layout(location = 2) flat out int v_ID;
layout(location = 3) out vec3 v_WorldPosition;
layout(location = 4) out vec3 v_CameraWorldPosition;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    v_WorldPosition = pos.xyz;
    v_CameraWorldPosition = u_SceneData.CameraPosition;
    v_Normal = normalize(mat3(u_MeshData.Transform) * a_Normal);
    v_UV = a_UV;
    v_ID = u_MeshData.ID;
}

#version 450 core
#pragma stage : Pixel

// begin_metadata
// set binding=0 UpdateFrequency::PerScene
// set binding=1 UpdateFrequency::PerMaterial
// end_metadata

layout(binding = 0) uniform sampler2D u_Albedo;

layout(std140, binding = 0) uniform LightData
{
    vec4 Color;
    vec3 Position;
    float Intensity;
} u_LightData;

layout(std140, binding = 1) uniform PBRData
{
    vec3 Albedo;
    float Padding;
    vec3 SpecularColor;
    float SpecularPower;
    float specualrIntensitiy;
    float Padding0;
    float Padding1;
    float Padding2;
} u_PBRData;

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_UV;
layout(location = 2) flat in int v_ID;
layout(location = 3) in vec3 v_WorldPosition;
layout(location = 4) in vec3 v_CameraWorldPosition;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

//const float Ambiant = 0.05f;
const float Ambiant = 0.0f;

float CalculateAttenuation(vec3 fragPos, vec3 lightPos)
{
    return (1.0f / length(lightPos - fragPos)) * u_LightData.Intensity;
}

void main()
{
    vec4 color = texture(u_Albedo, v_UV);
    color *= vec4(u_PBRData.Albedo, 1.0f);
    if (color.w <= 0.0f)
        discard;

    // Light
    vec3 dirToLight = normalize(u_LightData.Position - v_WorldPosition);
    float attenuation = CalculateAttenuation(v_WorldPosition, u_LightData.Position);
    float cosTheta = max(dot(v_Normal, dirToLight), 0.0f);
    vec3 radiance = u_LightData.Color.xyz * attenuation * cosTheta;

    color *= vec4(max(radiance, Ambiant), 1.0f);
    o_Color = vec4(color.xyz, 1.0f);

    //vec3 w = v_Normal * dot(dirToLight, v_Normal);
    //vec3 r = -(dirToLight - w) + w;
    //
    //vec3 sc = (pow(max(0.0f, dot(r, dirToCam)), 1.0f / u_PBRData.SpecularPower) * u_PBRData.SpecularColor) * u_PBRData.specualrIntensitiy;
    //
    //color *= max(clamp(scal * att, 0.0f, 1.0f), Ambiant);
    //color += vec4(sc, 1.0f);
    //
    //o_Color = vec4(color.xyz, 1.0f);
    o_ID = v_ID;
}
