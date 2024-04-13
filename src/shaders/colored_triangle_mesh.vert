#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std140) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(set = 0, binding = 0) uniform RenderableFixedData {
	mat4 model_matrix;
	VertexBuffer vertexBuffer;
} renderableData;

void main() 
{	
	//load vertex data from device adress
	Vertex v = renderableData.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = renderableData.model_matrix * vec4(v.position, 1.0f);
	outColor = v.color;
	outUV = vec2( v.uv_x, v.uv_y );
}