#ifndef _EAGE_VULKAN_TYPE_MAPPING_H_
#define _EAGE_VULKAN_TYPE_MAPPING_H_

///
/// Internal mapping between backend-agnostic GraphicsTypes and Vulkan types.
/// NEVER include this from a public header - only from .cpp files inside graphics/.
///

#include <vulkan/vulkan.hpp>
#include <graphics/GraphicsTypes.h>

namespace eage::graphics
{
	inline vk::Filter ToVulkan( TextureFilter f )
	{
		switch( f )
		{
			case TextureFilter::NEAREST: return vk::Filter::eNearest;
			case TextureFilter::LINEAR:  return vk::Filter::eLinear;
		}
		return vk::Filter::eNearest;
	}

	inline vk::BlendFactor ToVulkan( BlendFactor f )
	{
		switch( f )
		{
			case BlendFactor::ZERO:                  return vk::BlendFactor::eZero;
			case BlendFactor::ONE:                   return vk::BlendFactor::eOne;
			case BlendFactor::SRC_ALPHA:             return vk::BlendFactor::eSrcAlpha;
			case BlendFactor::ONE_MINUS_SRC_ALPHA:   return vk::BlendFactor::eOneMinusSrcAlpha;
			case BlendFactor::DST_ALPHA:             return vk::BlendFactor::eDstAlpha;
			case BlendFactor::ONE_MINUS_DST_ALPHA:   return vk::BlendFactor::eOneMinusDstAlpha;
		}
		return vk::BlendFactor::eOne;
	}

	inline vk::BlendOp ToVulkan( BlendOp op )
	{
		switch( op )
		{
			case BlendOp::ADD:               return vk::BlendOp::eAdd;
			case BlendOp::SUBTRACT:          return vk::BlendOp::eSubtract;
			case BlendOp::REVERSE_SUBTRACT:  return vk::BlendOp::eReverseSubtract;
		}
		return vk::BlendOp::eAdd;
	}

	inline vk::CompareOp ToVulkan( CompareOp op )
	{
		switch( op )
		{
			case CompareOp::NEVER:              return vk::CompareOp::eNever;
			case CompareOp::LESS:               return vk::CompareOp::eLess;
			case CompareOp::EQUAL:              return vk::CompareOp::eEqual;
			case CompareOp::LESS_OR_EQUAL:      return vk::CompareOp::eLessOrEqual;
			case CompareOp::GREATER:            return vk::CompareOp::eGreater;
			case CompareOp::NOT_EQUAL:          return vk::CompareOp::eNotEqual;
			case CompareOp::GREATER_OR_EQUAL:   return vk::CompareOp::eGreaterOrEqual;
			case CompareOp::ALWAYS:             return vk::CompareOp::eAlways;
		}
		return vk::CompareOp::eAlways;
	}

	inline vk::PrimitiveTopology ToVulkan( Topology t )
	{
		switch( t )
		{
			case Topology::POINT_LIST:      return vk::PrimitiveTopology::ePointList;
			case Topology::LINE_LIST:       return vk::PrimitiveTopology::eLineList;
			case Topology::LINE_STRIP:      return vk::PrimitiveTopology::eLineStrip;
			case Topology::TRIANGLE_LIST:   return vk::PrimitiveTopology::eTriangleList;
			case Topology::TRIANGLE_STRIP:  return vk::PrimitiveTopology::eTriangleStrip;
		}
		return vk::PrimitiveTopology::eTriangleList;
	}

	inline vk::PolygonMode ToVulkan( PolygonMode m )
	{
		switch( m )
		{
			case PolygonMode::FILL:   return vk::PolygonMode::eFill;
			case PolygonMode::LINE:   return vk::PolygonMode::eLine;
			case PolygonMode::POINT:  return vk::PolygonMode::ePoint;
		}
		return vk::PolygonMode::eFill;
	}

	inline vk::CullModeFlags ToVulkan( CullMode m )
	{
		switch( m )
		{
			case CullMode::NONE:            return vk::CullModeFlagBits::eNone;
			case CullMode::FRONT:           return vk::CullModeFlagBits::eFront;
			case CullMode::BACK:            return vk::CullModeFlagBits::eBack;
			case CullMode::FRONT_AND_BACK:  return vk::CullModeFlagBits::eFrontAndBack;
		}
		return vk::CullModeFlagBits::eNone;
	}

	inline vk::FrontFace ToVulkan( FrontFace f )
	{
		switch( f )
		{
			case FrontFace::COUNTER_CLOCKWISE:  return vk::FrontFace::eCounterClockwise;
			case FrontFace::CLOCKWISE:          return vk::FrontFace::eClockwise;
		}
		return vk::FrontFace::eClockwise;
	}

	inline vk::SamplerAddressMode ToVulkan( AddressMode m )
	{
		switch( m )
		{
			case AddressMode::REPEAT:            return vk::SamplerAddressMode::eRepeat;
			case AddressMode::MIRRORED_REPEAT:   return vk::SamplerAddressMode::eMirroredRepeat;
			case AddressMode::CLAMP_TO_EDGE:     return vk::SamplerAddressMode::eClampToEdge;
			case AddressMode::CLAMP_TO_BORDER:   return vk::SamplerAddressMode::eClampToBorder;
		}
		return vk::SamplerAddressMode::eRepeat;
	}

	inline vk::DescriptorType ToVulkan( UniformType t )
	{
		switch( t )
		{
			case UniformType::UNIFORM_BUFFER:  return vk::DescriptorType::eUniformBuffer;
			case UniformType::STORAGE_BUFFER:  return vk::DescriptorType::eStorageBuffer;
		}
		return vk::DescriptorType::eUniformBuffer;
	}
}

#endif // _EAGE_VULKAN_TYPE_MAPPING_H_
