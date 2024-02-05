#version 450 core
#pragma stage : Vertex

// begin_metadata
// set u_SceneData PerScene
// set u_MeshData PerDrawCall
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
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Bitangent;
layout(location = 4) in vec2 a_Texcoord;

struct VertexOutput
{
    vec3 WorldPosition;
    vec3 Normal;
    vec2 Texcoord;
    mat3 WorldNormals;

    vec3 ViewPosition;
};

layout(location = 0) out VertexOutput Output;
layout(location = 7) flat out int v_ID;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    Output.WorldPosition = pos.xyz;
    Output.ViewPosition = u_SceneData.CameraPosition;
    Output.Normal = normalize(mat3(u_MeshData.Transform) * a_Normal);
    Output.WorldNormals[0] = normalize(mat3(u_MeshData.Transform) * a_Tangent);
    Output.WorldNormals[1] = normalize(mat3(u_MeshData.Transform) * a_Bitangent);
    Output.WorldNormals[2] = normalize(mat3(u_MeshData.Transform) * a_Normal);
    Output.Texcoord = a_Texcoord;
    v_ID = u_MeshData.ID;
}

#version 450 core
#pragma stage : Pixel

// begin_metadata
// set u_MaterialUniforms PerMaterial
// set u_Light PerScene
// end_metadata

layout(binding = 0) uniform sampler2D u_AlbedoMap;
layout(binding = 1) uniform sampler2D u_NormalMap;
layout(binding = 2) uniform sampler2D u_MetalnessMap;
layout(binding = 3) uniform sampler2D u_RoughnessMap;
layout(binding = 4) uniform samplerCube u_IrradianceMap;

layout(std140, binding = 0) uniform MaterialUniforms
{
    vec3 Albedo;
    float Metalness;
    float Roughness;
    float AmbientOcclusion;
    bool UsingNormalMap;
    float Padding0;
} u_MaterialUniforms;

layout(std140, binding = 1) uniform Light
{
    vec4 Color;
    vec3 Position;
    float Intensity;
    float Radius;
	
    float P0, P1, P2;
} u_Light;

struct VertexOutput
{
    vec3 WorldPosition;
    vec3 Normal;
    vec2 Texcoord;
    mat3 WorldNormals;

    vec3 ViewPosition;
};

layout(location = 0) in VertexOutput Input;
layout(location = 7) in flat int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

struct Params
{
    vec3 Albedo;
    float Metalness;
    float Roughness;
    vec3 Normal;
    vec3 View;
    float NdotV;
} m_Params;

const vec3 Fdielectric = vec3(0.04);
const float PI = 3.14159265359;
const float Epsilon = 0.00001;

// GGX/Trowbride-Reitz normal distribution
// Uses Disney's reparameterization of alpha = Roughness^2
float NDFGGX(float cosLh)
{
    float alpha = m_Params.Roughness * m_Params.Roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

float SchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

float SchlickGGX(float cosLd, float cosLo)
{
    float r = m_Params.Roughness + 1;
    float k = (r * r) / 8;
    return SchlickG1(cosLd, k) * SchlickG1(cosLo, k);
}

vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 IBL(vec3 F0)
{
    vec3 irradiance = texture(u_IrradianceMap, m_Params.Normal).rgb;
    vec3 F = FresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
    
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), m_Params.Metalness);
    vec3 diffuseIBL = m_Params.Albedo * irradiance;

    // TODO(moro): specular

    return kd * diffuseIBL;
}

float LightFalloff(vec3 lightPosition, vec3 pixelPosition)
{
    vec3 distVec = lightPosition - pixelPosition;
    float LightDistance = sqrt(dot(distVec, distVec));
    float Dl = LightDistance / u_Light.Radius;
    float oneMinusDL = clamp(1.0 - (Dl * Dl * Dl * Dl), 0.0, 1.0);
    float numerator = oneMinusDL * oneMinusDL;
    return numerator / (LightDistance * LightDistance + 1.0);
}

void main()
{
    vec4 albedoTexColor = texture(u_AlbedoMap, Input.Texcoord);
    m_Params.Albedo = albedoTexColor.rgb * u_MaterialUniforms.Albedo;
    m_Params.Metalness = texture(u_MetalnessMap, Input.Texcoord).r * u_MaterialUniforms.Metalness;
    m_Params.Roughness = texture(u_RoughnessMap, Input.Texcoord).r * u_MaterialUniforms.Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05);

    m_Params.Normal = Input.Normal;
    if (u_MaterialUniforms.UsingNormalMap)
    {
        m_Params.Normal = normalize(texture(u_NormalMap, Input.Texcoord).rgb * 2.0 - 1.0);
        m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
    }

    m_Params.View = normalize(Input.ViewPosition - Input.WorldPosition);
    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

    vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

    // Direct Light
    vec3 lightContribution = vec3(0.0);
    {
        vec3 Ld = normalize(u_Light.Position - Input.WorldPosition);
        vec3 Lh = normalize(Ld + m_Params.View);

        //float cosLd = max(dot(m_Params.Normal, Ld));
        //float cosLh = max(dot(m_Params.Normal, Lh));
        float NdotL = max(dot(m_Params.Normal, Ld), 0.0);
        float NdotH = max(dot(m_Params.Normal, Lh), 0.0);

        // Calculate Fresnel term for direct lighting
        vec3 F = FresnelSchlick(F0, max(dot(Lh, m_Params.View), 0.0));

        // Calculate normal distribution for specular BRDF
        float D = NDFGGX(NdotH);

        // Calculate geometric attenuation for specular BRDF
        float G = SchlickGGX(NdotL, m_Params.NdotV);

        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), m_Params.Metalness);

        // Lambert diffuse BRDF
        vec3 diffuseBRDF = kd * m_Params.Albedo;

        // Cook-Torrance specular microfacet BRDF
        vec3 specularBRDF = (D * F * G) / max(4 * NdotL * m_Params.NdotV, Epsilon);
        
        float falloff = LightFalloff(u_Light.Position, Input.WorldPosition);
        float attenuation = min(u_Light.Intensity * falloff, 1);
        lightContribution += (diffuseBRDF + specularBRDF) * u_Light.Color.rgb * attenuation * NdotL;
    }

    vec3 iblContribution = IBL(F0);
    vec3 color = iblContribution + lightContribution;
    
    o_Color = vec4(color, 1.0);
    o_ID = v_ID;
}
