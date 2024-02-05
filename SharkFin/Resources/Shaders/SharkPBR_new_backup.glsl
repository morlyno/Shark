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
 
layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec3 v_Tangent;
layout(location = 3) out vec3 v_Bitangent;
layout(location = 4) out vec2 v_Texcoord;
layout(location = 5) out vec3 v_CameraWorldPosition;
layout(location = 6) flat out int v_ID;

void main()
{
    vec4 pos = u_MeshData.Transform * vec4(a_Position, 1.0f);
    gl_Position = u_SceneData.ViewProjection * pos;

    v_WorldPosition = pos.xyz;
    v_CameraWorldPosition = u_SceneData.CameraPosition;
    v_Normal = normalize(mat3(u_MeshData.Transform) * a_Normal);
    v_Tangent = normalize(mat3(u_MeshData.Transform) * a_Tangent);
    v_Bitangent = normalize(mat3(u_MeshData.Transform) * a_Bitangent);
    v_Texcoord = a_Texcoord;
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
    vec3 Radiance;
	float Padding;
} u_Light;

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec3 v_Tangent;
layout(location = 3) in vec3 v_Bitangent;
layout(location = 4) in vec2 v_Texcoord;
layout(location = 5) in vec3 v_CameraWorldPosition;
layout(location = 6) flat in int v_ID;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);
const float PI = 3.141592;
const float Epsilon = 0.00001;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    mat3 tanToWorld = mat3(
        v_Tangent,
        v_Bitangent,
        v_Normal
    );

	// Sample input textures to get shading model params.
	vec3 albedo = texture(u_AlbedoMap, v_Texcoord).rgb * u_MaterialUniforms.Albedo;
	float metalness = texture(u_MetalnessMap, v_Texcoord).r * u_MaterialUniforms.Metalness;
	float roughness = texture(u_RoughnessMap, v_Texcoord).r * u_MaterialUniforms.Roughness;

	// Outgoing light direction (vector from world-space fragment position to the "eye").
	vec3 Lo = normalize(v_CameraWorldPosition - v_WorldPosition);
	
    vec3 N;
    if (u_MaterialUniforms.UsingNormalMap)
    {
        N = normalize(2.0 * texture(u_NormalMap, v_Texcoord).xyz - 1.0);
        N = normalize(tanToWorld * N);
    }
    else
    {
        N = v_Normal;
    }

	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0, dot(N, Lo));
		
	// Specular reflection vector.
	vec3 Lr = 2.0 * cosLo * N - Lo;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	vec3 F0 = mix(Fdielectric, albedo, metalness);

	// Direct lighting calculation for analytical lights.
	vec3 directLighting = vec3(0);
	//for(int i=0; i<NumLights; ++i)
	{
		vec3 Li = normalize(u_Light.Position - v_WorldPosition);
		vec3 Lradiance = u_Light.Radiance;

		// Half-vector between Li and Lo.
		vec3 Lh = normalize(Li + Lo);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(N, Li));
		float cosLh = max(0.0, dot(N, Lh));

		// Calculate Fresnel term for direct lighting. 
		vec3 F  = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Calculate normal distribution for specular BRDF.
		float D = ndfGGX(cosLh, roughness);
		// Calculate geometric attenuation for specular BRDF.
		float G = gaSchlickGGX(cosLi, cosLo, roughness);

		// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);

		// Lambert diffuse BRDF.
		// We don't scale by 1/PI for lighting & material units to be more convenient.
		// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		vec3 diffuseBRDF = kd * albedo;

		// Cook-Torrance specular microfacet BRDF.
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

		// Total contribution for this light.
		directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}

	// Ambient lighting (IBL).
	vec3 ambientLighting = vec3(0.0);
	{
		// Sample diffuse irradiance at normal direction.
		vec3 irradiance = texture(u_IrradianceMap, N).rgb;

		// Calculate Fresnel term for ambient lighting.
		// Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
		// use cosLo instead of angle with light's half-vector (cosLh above).
		// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
		vec3 F = fresnelSchlick(F0, cosLo);

		// Get diffuse contribution factor (as with direct lighting).
		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);

		// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
		vec3 diffuseIBL = kd * albedo * irradiance;

		// Sample pre-filtered specular reflection environment at correct mipmap level.
		//int specularTextureLevels = textureQueryLevels(specularTexture);
		//vec3 specularIrradiance = textureLod(specularTexture, Lr, roughness * specularTextureLevels).rgb;

		// Split-sum approximation factors for Cook-Torrance specular BRDF.
		//vec2 specularBRDF = texture(specularBRDF_LUT, vec2(cosLo, roughness)).rg;

		// Total specular IBL contribution.
		//vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

		// Total ambient lighting contribution.
		//ambientLighting = diffuseIBL + specularIBL;
		ambientLighting = diffuseIBL;
	}

	// Final fragment color.
	o_Color = vec4(directLighting + ambientLighting, 1.0);
    o_ID = v_ID;
}
