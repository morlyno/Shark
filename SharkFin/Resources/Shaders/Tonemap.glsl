#version 450 core
#pragma stage : vertex

layout(location=0) in vec2 a_Position;

layout(location=0) out vec2 TexCoord;

void main()
{
	gl_Position = vec4(a_Position, 0.0, 1.0);
	TexCoord = a_Position.xy * 2.0 - 1.0;
}

#version 450 core
#pragma stage : pixel

layout(location=0) in vec2 TexCoord;
layout(location=0) out vec4 o_Color;

layout(set=1, binding=0) uniform sampler2D u_Input;

void main()
{
	vec3 color = texture(u_Input, TexCoord).rgb;
	
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 tv = color / (1.0f + color);
    vec3 mappedColor = mix(color / (1.0 + luminance), tv, tv);
    
	o_Color = vec4(mappedColor, 1.0);
}
