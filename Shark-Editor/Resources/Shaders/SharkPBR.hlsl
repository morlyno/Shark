#pragma stage : Vertex

// set 0 => Material
// set 1+ => RenderPass

struct Camera
{
    matrix ViewProjection;
    float3 Position;
    float Padding;
};

struct MeshData
{
    matrix Transform;
    int ID;
    float P0, P1, P2;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;
//[[vk::binding(1, 1)]] ConstantBuffer<MeshData> u_MeshData;
[[vk::push_constant]] ConstantBuffer<MeshData> u_MeshData;

struct VertexInput
{
    [[vk::location(0)]] float3 Position : Position;
    [[vk::location(1)]] float3 Normal : Normal;
    [[vk::location(2)]] float3 Tangent : Tangent;
    [[vk::location(3)]] float3 Bitangent : Bitangent;
    [[vk::location(4)]] float2 Texcoord : Texcoord;
};

struct VertexOutput
{
    [[vk::location(0)]] float3 WorldPosition : WorldPosition;
    [[vk::location(1)]] float3 Normal : Normal;
    [[vk::location(2)]] float2 Texcoord : Texcoord;
    [[vk::location(3)]] float3x3 WorldNormals : WorldNormals;
    [[vk::location(6)]] float3 ViewPosition : ViewPosition;
    [[vk::location(7)]] int ID : ID;
    float4 Position : SV_POSITION;
};

VertexOutput main(VertexInput Input)
{
    VertexOutput Output;
    
    float4 pos = mul(u_MeshData.Transform, float4(Input.Position, 1.0f));
    Output.Position = mul(u_Camera.ViewProjection, pos);

    Output.WorldPosition = pos.xyz;
    Output.ViewPosition = u_Camera.Position;
    Output.Normal = normalize(mul((float3x3)u_MeshData.Transform, Input.Normal));
    Output.WorldNormals[0] = normalize(mul((float3x3)u_MeshData.Transform, Input.Tangent));
    Output.WorldNormals[1] = normalize(mul((float3x3)u_MeshData.Transform, Input.Bitangent));
    Output.WorldNormals[2] = normalize(mul((float3x3)u_MeshData.Transform, Input.Normal));
    Output.Texcoord = Input.Texcoord;
    Output.ID = u_MeshData.ID;
    
    return Output;
}

#pragma stage : Pixel

struct MaterialUniforms
{
    float3 Albedo;
    float Metalness;
    float Roughness;
    float AmbientOcclusion;
    bool UsingNormalMap;
    float P0;
};

struct PointLight
{
    float3 Position;
    float Intensity;
    float3 Radiance;
    float Radius;
    float Falloff;
    float P0, P1, P2;
};

struct DirectionalLight
{
    float4 Radiance;
    float3 Direction;
    float Intensity;
};

struct Scene
{
    uint PointLightCount;
    uint DirectionalLightCount;
    float EnvironmentMapIntensity;
    float P0;
};

[[vk::binding(1, 1)]] ConstantBuffer<Scene> u_Scene;
[[vk::binding(2, 1)]] StructuredBuffer<PointLight> u_PointLights;
[[vk::binding(3, 1)]] StructuredBuffer<DirectionalLight> u_DirectionalLights;

#define SAMPLER(_textureName) _ ## _textureName ## _sampler

[[vk::binding(4, 1)]][[vk::combinedImageSampler]] uniform TextureCube u_IrradianceMap;
[[vk::binding(4, 1)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_IrradianceMap);

[[vk::binding(5, 1)]][[vk::combinedImageSampler]] uniform TextureCube u_RadianceMap;
[[vk::binding(5, 1)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_RadianceMap);

[[vk::binding(6, 1)]][[vk::combinedImageSampler]] uniform Texture2D u_BRDFLUTTexture;
[[vk::binding(6, 1)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_BRDFLUTTexture);


[[vk::binding(0, 0)]] ConstantBuffer<MaterialUniforms> u_MaterialUniforms;

[[vk::binding(1, 0)]][[vk::combinedImageSampler]] uniform Texture2D u_AlbedoMap;
[[vk::binding(1, 0)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_AlbedoMap);

[[vk::binding(2, 0)]][[vk::combinedImageSampler]] uniform Texture2D u_NormalMap;
[[vk::binding(2, 0)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_NormalMap);

[[vk::binding(3, 0)]][[vk::combinedImageSampler]] uniform Texture2D u_MetalnessMap;
[[vk::binding(3, 0)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_MetalnessMap);

[[vk::binding(4, 0)]][[vk::combinedImageSampler]] uniform Texture2D u_RoughnessMap;
[[vk::binding(4, 0)]][[vk::combinedImageSampler]] uniform SamplerState SAMPLER(u_RoughnessMap);


struct PixelInput
{
    [[vk::location(0)]] float3 WorldPosition : WorldPosition;
    [[vk::location(1)]] float3 Normal : Normal;
    [[vk::location(2)]] float2 Texcoord : Texcoord;
    [[vk::location(3)]] float3x3 WorldNormals : WorldNormals;
    [[vk::location(6)]] float3 ViewPosition : ViewPosition;
    [[vk::location(7)]] int ID : ID;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
};

struct Params
{
    float3 Albedo;
    float Metalness;
    float Roughness;
    float3 Normal;
    float3 View;
    float NdotV;
};
static Params m_Params;

static const float3 Fdielectric = (float3)0.04;
static const float PI = 3.14159265359;
static const float Epsilon = 0.00001;

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

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 FresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
    return F0 + (max((float3)(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 IBL(float3 F0)
{
    float3 irradiance = u_IrradianceMap.Sample(SAMPLER(u_IrradianceMap), m_Params.Normal).rgb;
    float3 F = FresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
    
    float3 kd = lerp((float3)1.0 - F, (float3)0.0, m_Params.Metalness);
    float3 diffuseIBL = m_Params.Albedo * irradiance;

    uint width, height, radianceTexLevels;
    u_RadianceMap.GetDimensions(0, width, height, radianceTexLevels);
    float3 R = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
    float3 specularIrradiance = u_RadianceMap.SampleLevel(SAMPLER(u_RadianceMap), R, m_Params.Roughness * radianceTexLevels);
    
    float2 specularBRDF = u_BRDFLUTTexture.Sample(SAMPLER(u_BRDFLUTTexture), float2(m_Params.NdotV, m_Params.Roughness)).rg;
    float3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
    
    return kd * diffuseIBL + specularIBL;
    //return irradiance;
}

float LightAttenuation(float distance, float intensity, float radius, float falloff)
{
    float s = distance / radius;
    
    if (s >= 1.0f)
    {
        return 0.0f;
    }

    float s2 = s * s;
    float oneMinusS2 = 1.0f - s2;

    return intensity * (oneMinusS2 * oneMinusS2) / (1 + falloff * s);
}

float4 ToLinear(float4 sRGB)
{
    bool4 cutoff = sRGB < (float4)0.04045;
    float4 higher = pow((sRGB + 0.055) / 1.055, 2.4);
    float4 lower = sRGB / 12.92;
    return lerp(higher, lower, cutoff);
}

PixelOutput main(PixelInput Input)
{
    float4 albedoTexColor = u_AlbedoMap.Sample(SAMPLER(u_AlbedoMap), Input.Texcoord);
    m_Params.Albedo = albedoTexColor.rgb * ToLinear(float4(u_MaterialUniforms.Albedo, 1.0f)).rgb;
    //m_Params.Albedo = albedoTexColor.rgb * u_MaterialUniforms.Albedo;
    m_Params.Metalness = u_MetalnessMap.Sample(SAMPLER(u_MetalnessMap), Input.Texcoord).b * u_MaterialUniforms.Metalness;
    m_Params.Roughness = u_RoughnessMap.Sample(SAMPLER(u_RoughnessMap), Input.Texcoord).g * u_MaterialUniforms.Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05);

    m_Params.Normal = Input.Normal;
    if (u_MaterialUniforms.UsingNormalMap)
    {
        m_Params.Normal = normalize(u_NormalMap.Sample(SAMPLER(u_NormalMap), Input.Texcoord).rgb * 2.0 - 1.0);
        m_Params.Normal = normalize(mul(m_Params.Normal, Input.WorldNormals));
    }

    m_Params.View = normalize(Input.ViewPosition - Input.WorldPosition);
    m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

    float3 F0 = lerp(Fdielectric, m_Params.Albedo, m_Params.Metalness);

    // Direct Light
    float3 lightContribution = 0.0f;
    
    //if (u_Scene.LightCount > 0)
    const uint MaxPointLights = 256;
    for (uint pointLightIndex = 0; pointLightIndex < u_Scene.PointLightCount && pointLightIndex < MaxPointLights; pointLightIndex++)
    {
        PointLight light = u_PointLights[pointLightIndex];
        
        float3 Ld = normalize(light.Position - Input.WorldPosition);
        float3 Lh = normalize(Ld + m_Params.View);

        //float cosLd = max(dot(m_Params.Normal, Ld));
        //float cosLh = max(dot(m_Params.Normal, Lh));
        float NdotL = max(dot(m_Params.Normal, Ld), 0.0);
        float NdotH = max(dot(m_Params.Normal, Lh), 0.0);

        // Calculate Fresnel term for direct lighting
        float3 F = FresnelSchlick(F0, max(dot(Lh, m_Params.View), 0.0));

        // Calculate normal distribution for specular BRDF
        float D = NDFGGX(NdotH);

        // Calculate geometric attenuation for specular BRDF
        float G = SchlickGGX(NdotL, m_Params.NdotV);

        float3 kd = lerp(1.0 - F, 0.0, m_Params.Metalness);

        // Lambert diffuse BRDF
        float3 diffuseBRDF = kd * m_Params.Albedo;

        // Cook-Torrance specular microfacet BRDF
        float3 specularBRDF = (D * F * G) / max(4 * NdotL * m_Params.NdotV, Epsilon);
        
        float lightDist = distance(light.Position, Input.WorldPosition);
        float attenuation = LightAttenuation(lightDist, light.Intensity, light.Radius, light.Falloff);
        //attenuation = min(attenuation, 1.0f);

        //float falloff = LightFalloff(u_Light.Position, Input.WorldPosition);
        //float attenuation = u_Light.Intensity * falloff;

        lightContribution += (diffuseBRDF + specularBRDF) * light.Radiance.rgb * attenuation * NdotL;
    }
    
    // Directional Light
    uint MaxDirectionalLights = 5;
    for (uint dirLightIndex = 0; dirLightIndex < u_Scene.DirectionalLightCount && dirLightIndex < MaxDirectionalLights; dirLightIndex++)
    {
        DirectionalLight dirLight = u_DirectionalLights[dirLightIndex];
        float3 Ld = normalize(dirLight.Direction);
        float3 Lh = normalize(Ld + m_Params.View);

        //float cosLd = max(dot(m_Params.Normal, Ld));
        //float cosLh = max(dot(m_Params.Normal, Lh));
        float NdotL = max(dot(m_Params.Normal, Ld), 0.0);
        float NdotH = max(dot(m_Params.Normal, Lh), 0.0);

        // Calculate Fresnel term for direct lighting
        float3 F = FresnelSchlick(F0, max(dot(Lh, m_Params.View), 0.0));

        // Calculate normal distribution for specular BRDF
        float D = NDFGGX(NdotH);

        // Calculate geometric attenuation for specular BRDF
        float G = SchlickGGX(NdotL, m_Params.NdotV);

        float3 kd = lerp(1.0 - F, 0.0, m_Params.Metalness);

        // Lambert diffuse BRDF
        float3 diffuseBRDF = kd * m_Params.Albedo;

        // Cook-Torrance specular microfacet BRDF
        float3 specularBRDF = (D * F * G) / max(4 * NdotL * m_Params.NdotV, Epsilon);
        
        float attenuation = dirLight.Intensity;
        //attenuation = min(attenuation, 1.0f);

        //float falloff = LightFalloff(u_Light.Position, Input.WorldPosition);
        //float attenuation = u_Light.Intensity * falloff;

        lightContribution += (diffuseBRDF + specularBRDF) * dirLight.Radiance.rgb * attenuation * NdotL;
    }
    
    float3 iblContribution = IBL(F0) * u_Scene.EnvironmentMapIntensity;
    float3 color = iblContribution + lightContribution;
    
    PixelOutput Output;
    Output.Color = float4(color, 1.0);
    Output.ID = Input.ID;
    return Output;
}
