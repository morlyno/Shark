#version 450 core
#pragma stage : Vertex

// begin_metadata
// set binding=0 UpdateFrequency::PerScene
// set binding=1 UpdateFrequency::PerDrawCall
// end_metadata

layout(std140, binding = 0) uniform SceneData
{
    mat4 ViewProjection;
    vec3 CameraPosition;
    float Padding;
} u_SceneData;

layout(std140, binding = 1) uniform MeshData
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
 
layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_UV;
layout(location = 3) out vec3 v_CameraWorldPosition;
layout(location = 4) flat out int v_ID;

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
// set binding=0 UpdateFrequency::PerMaterial
// set binding=1 UpdateFrequency::PerScene
// end_metadata

layout(binding = 0) uniform sampler2D u_Albedo;

layout(std140, binding = 0) uniform PBR
{
    vec3 Albedo;
    float Padding0;
    float Metallic;
    float Roughness;
    float AO;
    float Padding1;
} u_PBR;

layout(std140, binding = 1) uniform Light
{
    vec4 Color;
    vec3 Position;
    float Intensity;
} u_Light;

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_CameraWorldPosition;
layout(location = 4) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(v_CameraWorldPosition - v_WorldPosition);

    vec3 F0 = vec3(0.04);

    vec3 albedo = u_PBR.Albedo * texture(u_Albedo, v_UV).xyz;
    F0 = mix(F0, albedo.xyz, u_PBR.Metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    //for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        //vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 L = normalize(u_Light.Position - v_WorldPosition);
        vec3 H = normalize(V + L);
        //float distance    = length(lightPositions[i] - WorldPos);
        float distance    = length(u_Light.Position - v_WorldPosition);
        //float attenuation = 1.0 / (distance * distance);
        float attenuation = (1.0 / (distance * distance)) * u_Light.Intensity;
        //vec3 radiance     = lightColors[i] * attenuation;
        vec3 radiance     = u_Light.Color.xyz * attenuation;
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, u_PBR.Roughness);
        float G   = GeometrySmith(N, V, L, u_PBR.Roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - u_PBR.Metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * u_PBR.AO;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    o_Color = vec4(color, 1.0);
    o_ID = v_ID;
}
