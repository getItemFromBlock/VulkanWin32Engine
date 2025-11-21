#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject
{
    vec2 invRes;
	vec2 scale;
} ubo;

layout(std140, binding = 1) readonly buffer ObjectBuffer
{
	vec4 objects[];
} objectBuffer;

layout (location = 0) out vec2 fragUV;
layout (location = 1) out vec3 fragColor;

void main()
{
	vec4 objectData = objectBuffer.objects[gl_InstanceIndex];
	float cr = cos(objectData.z);
	float sr = sin(objectData.z);
	vec2 dest = vec2(	inPosition.x * cr + inPosition.y * sr,
						inPosition.y * cr - inPosition.x * sr);
	dest = (dest * ubo.scale + objectData.xy) * ubo.invRes * 2.0 - 1.0;
	gl_Position = vec4(dest, inPosition.z, 1.0);
	fragUV = inUV;
	fragColor = inColor;
}
