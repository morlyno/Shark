#version 450 core
#pragma stage : vertex

layout(location=0) in vec2 a_Position;

layout(location=0) out vec2 TexCoord;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
	TexCoord = (a_Position.xy + 1.0) * 0.5;
	TexCoord.y = 1.0 - TexCoord.y;
}

#version 450 core
#pragma stage : pixel

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 o_Color;

layout(set=1, binding=0) uniform Settings
{
	bool Tonemap;
	float Exposure;
	float P0, P1;
} u_Settings;

layout(set=1, binding=1) uniform sampler2D u_Input;

void main()
{
	vec3 color = texture(u_Input, TexCoord).rgb * u_Settings.Exposure;

	if (u_Settings.Tonemap)
	{
		float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
		vec3 tv = color / (1.0f + color);
		color = mix(color / (1.0 + luminance), tv, tv);
	}

	o_Color = vec4(color, 1.0);
}
