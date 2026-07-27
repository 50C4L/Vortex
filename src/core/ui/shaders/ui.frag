#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout( location = 0 ) in vec4 inColor;
layout( location = 1 ) in vec2 inUV;
layout( location = 2 ) flat in uint inHasTexture;
layout( location = 3 ) flat in uint inTexIndex;

layout( location = 0 ) out vec4 outFragColor;

layout( set = 0, binding = 0 ) uniform sampler2D textures[];

void main()
{
	if( inHasTexture != 0u )
	{
		outFragColor = texture( textures[ nonuniformEXT( inTexIndex ) ], inUV ) * inColor;
	}
	else
	{
		outFragColor = inColor;
	}
}
