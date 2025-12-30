#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormal;
layout (location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

const vec3 lightDir = normalize(vec3(0.9,-2,1.1));

void main()
{
	//outColor = vec4(fragColor*vec3(fragUV,1.0), 1.0);
	//outColor = vec4(vec3(fragUV,0.0), 1.0);
	/*
	vec3 c;
	int s = int((fragUV.y + 0.05) * 7);
	if (s == 3)
		c = vec3(0.933, 0.933, 0.933);
	else if (s > 1 && s < 5)
		c = vec3(0.961, 0.663, 0.722);
	else
		c = vec3(0.357, 0.808, 0.980);
	outColor = vec4(pow(c, vec3(2.4)), 1.0);
	
	float fact = pow(1.0 - fragUV.y, 2.0);
	outColor = vec4(vec3(1.0, fact, fact), 1.0);
	float flip = fragNormal.x + fragNormal.y + fragNormal.z;
	outColor = vec4(fragUV, 1-flip, 1.0);
	*/
	vec2 uv = vec2(fragUV.y, -fragUV.x);
	vec3 c = texture(texSampler, uv).xyz;
	
	float f = clamp(dot(-lightDir,fragNormal),0,1);
	f = 0.5 + f * 0.7;
	outColor = vec4(clamp(c*f,0,1), 1.0);
}
