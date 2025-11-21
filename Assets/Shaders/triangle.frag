#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

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
	*/
	float fact = pow(1.0 - fragUV.y, 2.0);
	outColor = vec4(vec3(1.0, fact, fact), 1.0);
}
