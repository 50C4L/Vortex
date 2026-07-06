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

layout(set = 0, binding = 0) uniform SceneGlobalData {
	mat4 view_matrix;
	mat4 project_matrix;
	mat4 project_view_matrix;
	vec2 virtual_resolution;
} sceneGlobalData;

layout(set = 1, binding = 0) uniform RenderableFixedData {
	mat4 model_matrix;
	VertexBuffer vertexBuffer; // 8 bytes
	// 8 bytes implicit padding to align vec4 to offset 80
	vec4 uv_rect; // xy = uv offset, zw = uv scale
	uint texture_index;
} renderableData;

layout(location = 2) flat out uint outTexIndex;

void main() 
{	
	//load vertex data from device adress
	Vertex v = renderableData.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = sceneGlobalData.project_view_matrix * renderableData.model_matrix * vec4(v.position, 1.0f);

	// Snap to virtual pixel grid to preserve pixel-art alignment during rotation
	vec2 half_res = sceneGlobalData.virtual_resolution * 0.5;
	vec2 ndc = gl_Position.xy / gl_Position.w;
	vec2 snapped = round( ndc * half_res ) / half_res;
	gl_Position.xy = snapped * gl_Position.w;

	outColor = v.color;
	outUV = vec2( v.uv_x, v.uv_y ) * renderableData.uv_rect.zw + renderableData.uv_rect.xy;
	outTexIndex = renderableData.texture_index;
}