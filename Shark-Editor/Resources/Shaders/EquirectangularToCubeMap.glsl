#version 450 core
#pragma stage : compute

#include <EnvironmentMapping.glslh>

//layout(binding = 0, rgba16f) restrict writeonly uniform imageCube o_CubeMap;
layout(set = 0, binding = 0, rgba16f) restrict writeonly uniform image2DArray o_CubeMap;
layout(set = 0, binding = 1) uniform sampler2D u_EquirectangularTex;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
	vec3 cubeTC = GetCubeMapTexCoord(vec2(imageSize(o_CubeMap)));
	
	float phi = atan(cubeTC.z, cubeTC.x);
	float theta = acos(cubeTC.y);
	vec2 uv = vec2(phi / (TwoPI) + 0.5, theta / PI);

	vec4 color = texture(u_EquirectangularTex, uv);
	color = min(color, vec4(100.0f));
	imageStore(o_CubeMap, ivec3(gl_GlobalInvocationID), color);
}
