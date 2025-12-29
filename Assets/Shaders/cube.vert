#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

struct Object {
    vec3 position;
	float padding0;
    vec3 velocity;
	float padding1;
	vec3 accel;
	float padding2;
    vec4 rotation;
};

layout(binding = 0) uniform UniformBufferObject
{
    mat4 vp;
} ubo;

layout(std140, binding = 1) readonly buffer ObjectBuffer
{
	Object objects[];
};

layout (location = 0) out vec2 fragUV;
layout (location = 1) out vec3 fragColor;
layout (location = 2) out vec3 fragNormal;



vec4 QuatMul(vec4 a, vec4 other)
{
	return vec4(other.xyz * a.w + a.xyz * other.w + cross(a.xyz, other.xyz), a.w*other.w - dot(a.xyz, other.xyz));
}

vec4 QuatInverse(vec4 a)
{
	return normalize(vec4(-a.xyz, a.w));
}

vec3 QuatMul(vec4 a, vec3 other)
{
	vec4 tmp = QuatMul(QuatMul(a, vec4(other, 0.0)), QuatInverse(a));
	return tmp.xyz;
}

void main()
{
	Object data = objects[gl_InstanceIndex];
	vec3 dest = QuatMul(data.rotation, inPosition);
	dest += data.position;
	gl_Position = vec4(dest, 1.0) * ubo.vp;

	fragUV = inUV;
	fragColor = inColor;
	fragNormal = QuatMul(data.rotation, inNormal);
}
