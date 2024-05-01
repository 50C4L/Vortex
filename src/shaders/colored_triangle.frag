#version 450

//shader input
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 2, binding = 0) uniform sampler2D texSampler;

void main() 
{
	//return red
	outFragColor = texture(texSampler, inUV);
}