#version 450 core
#pragma stage : compute

#include <EnvironmentMapping.glslh>

const uint NumSamples = 1024;
const float InvNumSamples = 1.0 / float(NumSamples);

const int NumMipLevels = 1;
layout(binding=0, rgba32f) restrict writeonly uniform image2DArray outputTexture;
layout(binding=1) uniform samplerCube inputTexture;

layout (push_constant) uniform Uniforms
{
	float Roughness;
	float P0, P1, P2;
} u_Uniforms;

#define PARAM_ROUGHNESS u_Uniforms.Roughness

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;
void main()
{
	// Make sure we won't write past output when computing higher mipmap levels.
	ivec2 outputSize = ivec2(imageSize(outputTexture));
	if(gl_GlobalInvocationID.x >= outputSize.x || gl_GlobalInvocationID.y >= outputSize.y) {
		return;
	}
	
	// Solid angle associated with a single cubemap texel at zero mipmap level.
	// This will come in handy for importance sampling below.
	vec2 inputSize = vec2(textureSize(inputTexture, 0));
	float wt = 4.0 * PI / (6 * inputSize.x * inputSize.y);
	
	// Approximation: Assume zero viewing angle (isotropic reflections).
	vec3 N = GetCubeMapTexCoord(vec2(imageSize(outputTexture)));
	vec3 Lo = N;
	
	vec3 S, T;
	ComputeBasisVectors(N, S, T);

	vec3 color = vec3(0);
	float weight = 0;

	// Convolve environment map using GGX NDF importance sampling.
	// Weight by cosine term since Epic claims it generally improves quality.
	for(uint i=0; i<NumSamples; ++i) {
		vec2 u = SampleHammersley(i, NumSamples);
		vec3 Lh = TangentToWorld(SampleGGX(u.x, u.y, PARAM_ROUGHNESS), N, S, T);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
		vec3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

		float cosLi = dot(N, Li);
		if(cosLi > 0.0) {
			// Use Mipmap Filtered Importance Sampling to improve convergence.
			// See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

			float cosLh = max(dot(N, Lh), 0.0);

			// GGX normal distribution function (D term) probability density function.
			// Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
			float pdf = NDFGGX(cosLh, PARAM_ROUGHNESS) * 0.25;

			// Solid angle associated with this sample.
			float ws = 1.0 / (NumSamples * pdf);

			// Mip level to sample from.
			float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

			color  += textureLod(inputTexture, Li, mipLevel).rgb * cosLi;
			weight += cosLi;
		}
	}
	color /= weight;

	imageStore(outputTexture, ivec3(gl_GlobalInvocationID), vec4(color, 1.0));
}