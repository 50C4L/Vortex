#include "BuiltInMeshes.h"

eage::graphics::BuiltInRect 
eage::graphics::made_rect_vertices( glm::vec3 center, float width, float height )
{
	BuiltInRect rect;
	rect.vertices.resize( 4 );
	rect.indices.resize( 6 );

	rect.vertices[0].position = center + glm::vec3(  width / 2, -height / 2, 0 );
	rect.vertices[1].position = center + glm::vec3(  width / 2,  height / 2, 0 );
	rect.vertices[2].position = center + glm::vec3( -width / 2, -height / 2, 0 );
	rect.vertices[3].position = center + glm::vec3( -width / 2,  height / 2, 0 );

	rect.vertices[0].color = { 1, 1, 1, 1 };
	rect.vertices[1].color = { 1, 1, 1, 1 };
	rect.vertices[2].color = { 1, 1, 1, 1 };
	rect.vertices[3].color = { 1, 1, 1, 1 };

	rect.vertices[0].uv_x = 1.f;
	rect.vertices[0].uv_y = 0.f;
	rect.vertices[1].uv_x = 1.f;
	rect.vertices[1].uv_y = 1.f;
	rect.vertices[2].uv_x = 0.f;
	rect.vertices[2].uv_y = 0.f;
	rect.vertices[3].uv_x = 0.f;
	rect.vertices[3].uv_y = 1.f;

	rect.indices[0] = 0;
	rect.indices[1] = 1;
	rect.indices[2] = 2;

	rect.indices[3] = 2;
	rect.indices[4] = 1;
	rect.indices[5] = 3;

	return rect;
}
