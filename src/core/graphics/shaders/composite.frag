#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout( location = 0 ) in vec2 inUV;
layout( location = 0 ) out vec4 outFragColor;

layout( set = 0, binding = 0 ) uniform sampler2D textures[];

layout( push_constant ) uniform PushConstants
{
	uint base_index;
	uint overlay_index;
} push;

void main()
{
	vec4 base = texture( textures[ nonuniformEXT( push.base_index ) ], inUV );
	vec4 overlay = texture( textures[ nonuniformEXT( push.overlay_index ) ], inUV );
	outFragColor = vec4( overlay.rgb * overlay.a + base.rgb * ( 1.0 - overlay.a ), 1.0 );
}
