#version 450
#extension GL_EXT_buffer_reference : require

layout( location = 0 ) out vec4 outColor;
layout( location = 1 ) out vec2 outUV;
layout( location = 2 ) flat out uint outHasTexture;
layout( location = 3 ) flat out uint outTexIndex;

struct UIVertex
{
	vec2 position;
	vec2 tex_coord;
	vec4 colour;
};

layout( buffer_reference, std430 ) readonly buffer VertexBuffer
{
	UIVertex vertices[];
};

layout( push_constant ) uniform PushConstants
{
	vec2 scale;
	vec2 translation;
	VertexBuffer vertex_buffer;
	uint texture_index;
	uint has_texture;
} push;

void main()
{
	UIVertex v = push.vertex_buffer.vertices[ gl_VertexIndex ];
	vec2 translated = v.position + push.translation;
	gl_Position = vec4( translated * push.scale + vec2( -1.0, -1.0 ), 0.0, 1.0 );
	outColor = v.colour;
	outUV = v.tex_coord;
	outHasTexture = push.has_texture;
	outTexIndex = push.texture_index;
}
