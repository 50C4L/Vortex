#ifndef _EAGE_GRAPHICS_TYPES_H_
#define _EAGE_GRAPHICS_TYPES_H_

#include <cstdint>

namespace eage::graphics
{
	enum class TextureFilter : uint8_t
	{
		NEAREST,
		LINEAR
	};

	enum class BlendFactor : uint8_t
	{
		ZERO,
		ONE,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA
	};

	enum class BlendOp : uint8_t
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT
	};

	enum class CompareOp : uint8_t
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS
	};

	enum class Topology : uint8_t
	{
		POINT_LIST,
		LINE_LIST,
		LINE_STRIP,
		TRIANGLE_LIST,
		TRIANGLE_STRIP
	};

	enum class PolygonMode : uint8_t
	{
		FILL,
		LINE,
		POINT
	};

	enum class CullMode : uint8_t
	{
		NONE,
		FRONT,
		BACK,
		FRONT_AND_BACK
	};

	enum class FrontFace : uint8_t
	{
		COUNTER_CLOCKWISE,
		CLOCKWISE
	};

	enum class AddressMode : uint8_t
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class UniformType : uint8_t
	{
		UNIFORM_BUFFER,
		STORAGE_BUFFER
	};
}

#endif // _EAGE_GRAPHICS_TYPES_H_
