#version 450 core
#pragma stage : vertex

layout(location=0) in vec3 a_Position;
layout(location=1) in vec2 a_TexCoord;

layout(location=0) out vec2 o_TexCoord;

void main()
{
	gl_Position = vec4(a_Position, 1.0f);
	o_TexCoord = a_TexCoord;
}

#version 450 core
#pragma stage : pixel

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 o_Color;

layout(set=1, binding=0) uniform Settings
{
	bool Tonemap;
	bool GammaCorrect;
	float Exposure;
	float P0;
} u_Settings;

layout(set=1, binding=1) uniform sampler2D u_Input;

// ACES Tonemap
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 Tonemap(vec3 color)
{
	mat3 m1 = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);
	mat3 m2 = mat3(
		 1.60475, -0.10208, -0.00327,
		-0.53108,  1.10813, -0.07276,
		-0.07367, -0.00605,  1.07602
	);

	vec3 v = m1 * color;
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

vec3 GammaCorrect(vec3 color, float gamma)
{
	return pow(color, vec3(1.0 / gamma));
}

void main()
{
	const float gamma = 2.2;
	const float pureWhite = 1.0;
	float samplesScale = 0.5;

	vec3 color = texture(u_Input, TexCoord).rgb;

	color *= u_Settings.Exposure;

	if (u_Settings.Tonemap)
	{
		color = Tonemap(color);
	}

	if (u_Settings.GammaCorrect)
	{
		color = GammaCorrect(color, gamma);
	}

	o_Color = vec4(color, 1.0);
}
