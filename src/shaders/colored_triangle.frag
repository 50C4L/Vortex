#version 450
#extension GL_EXT_nonuniform_qualifier : require

//shader input
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) flat in uint inTexIndex;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 2, binding = 0) uniform sampler2D textures[];

void main() 
{
	outFragColor = texture( textures[nonuniformEXT(inTexIndex)], inUV );
}