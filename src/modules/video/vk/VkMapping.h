/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "flextVk.h"
#include "video/Types.h"

namespace video {
namespace _priv {

static const VkCompareOp CompareFuncs[] = {
	VK_COMPARE_OP_NEVER,			// Never
	VK_COMPARE_OP_LESS,				// Less
	VK_COMPARE_OP_EQUAL,			// Equal
	VK_COMPARE_OP_LESS_OR_EQUAL,	// LessEqual
	VK_COMPARE_OP_GREATER,			// Greater
	VK_COMPARE_OP_NOT_EQUAL,		// NotEqual
	VK_COMPARE_OP_GREATER_OR_EQUAL, // GreaterOrEqual
	VK_COMPARE_OP_ALWAYS			// Always
};
static_assert(core::enumVal(CompareFunc::Max) == lengthof(CompareFuncs), "Array sizes don't match Max");

static const VkBlendFactor BlendModes[] = {
	VK_BLEND_FACTOR_ZERO,				 // Zero
	VK_BLEND_FACTOR_ONE,				 // One
	VK_BLEND_FACTOR_SRC_COLOR,			 // SourceColor
	VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // OneMinusSourceColor
	VK_BLEND_FACTOR_SRC_ALPHA,			 // SourceAlpha
	VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // OneMinusSourceAlpha
	VK_BLEND_FACTOR_DST_ALPHA,			 // DestinationAlpha
	VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // OneMinusDestinationAlpha
	VK_BLEND_FACTOR_DST_COLOR,			 // DestinationColor
	VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR	 // OneMinusDestinationColor
};
static_assert(core::enumVal(BlendMode::Max) == lengthof(BlendModes), "Array sizes don't match Max");

static const VkBlendOp BlendEquations[] = {
	VK_BLEND_OP_ADD,			  // Add
	VK_BLEND_OP_SUBTRACT,		  // Subtract
	VK_BLEND_OP_REVERSE_SUBTRACT, // ReverseSubtract
	VK_BLEND_OP_MIN,			  // Minimum
	VK_BLEND_OP_MAX				  // Maximum
};
static_assert(core::enumVal(BlendEquation::Max) == lengthof(BlendEquations), "Array sizes don't match Max");

static const VkCullModeFlagBits CullModes[] = {
	VK_CULL_MODE_FRONT_BIT,		// Front
	VK_CULL_MODE_BACK_BIT,		// Back
	VK_CULL_MODE_FRONT_AND_BACK // FrontAndBack
};
static_assert(core::enumVal(Face::Max) == lengthof(CullModes), "Array sizes don't match Max");

static const VkPolygonMode PolygonModes[] = {
	VK_POLYGON_MODE_POINT, // Points
	VK_POLYGON_MODE_LINE,  // WireFrame
	VK_POLYGON_MODE_FILL   // Solid
};
static_assert(core::enumVal(PolygonMode::Max) == lengthof(PolygonModes), "Array sizes don't match Max");

static const VkPrimitiveTopology PrimitiveTopologies[] = {
	VK_PRIMITIVE_TOPOLOGY_POINT_LIST,					// Points
	VK_PRIMITIVE_TOPOLOGY_LINE_LIST,					// Lines
	VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,		// LinesAdjacency
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,				// Triangles
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, // TrianglesAdjacency
	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,					// LineStrip
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP				// TriangleStrip
};
static_assert(core::enumVal(Primitive::Max) == lengthof(PrimitiveTopologies), "Array sizes don't match Max");

static const VkShaderStageFlagBits ShaderStages[] = {
	VK_SHADER_STAGE_VERTEX_BIT,	  // Vertex
	VK_SHADER_STAGE_FRAGMENT_BIT, // Fragment
	VK_SHADER_STAGE_GEOMETRY_BIT, // Geometry
	VK_SHADER_STAGE_COMPUTE_BIT	  // Compute
};
static_assert(core::enumVal(ShaderType::Max) == lengthof(ShaderStages), "Array sizes don't match Max");

static const VkFormat TextureFormats[] = {
	VK_FORMAT_R8G8B8A8_UNORM,	   // RGBA
	VK_FORMAT_R8G8B8_UNORM,		   // RGB
	VK_FORMAT_R32G32B32A32_SFLOAT, // RGBA32F
	VK_FORMAT_R32G32B32_SFLOAT,	   // RGB32F
	VK_FORMAT_R16G16B16A16_SFLOAT, // RGBA16F
	VK_FORMAT_D24_UNORM_S8_UINT,   // D24S8
	VK_FORMAT_D32_SFLOAT_S8_UINT,  // D32FS8
	VK_FORMAT_D24_UNORM_S8_UINT,   // D24 (approximation)
	VK_FORMAT_D32_SFLOAT,		   // D32F
	VK_FORMAT_S8_UINT,			   // S8
	VK_FORMAT_R16G16_UINT		   // RG16U
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(TextureFormats), "Array sizes don't match Max");

static const VkImageType ImageTypes[] = {
	VK_IMAGE_TYPE_1D, // Texture1D
	VK_IMAGE_TYPE_2D, // Texture2D
	VK_IMAGE_TYPE_2D, // Texture2DArray
	VK_IMAGE_TYPE_2D, // Texture2DMultisample
	VK_IMAGE_TYPE_2D, // Texture2DMultisampleArray
	VK_IMAGE_TYPE_3D, // Texture3D
	VK_IMAGE_TYPE_2D  // TextureCube
};
static_assert(core::enumVal(TextureType::Max) == lengthof(ImageTypes), "Array sizes don't match Max");

static const VkImageViewType ImageViewTypes[] = {
	VK_IMAGE_VIEW_TYPE_1D,		 // Texture1D
	VK_IMAGE_VIEW_TYPE_2D,		 // Texture2D
	VK_IMAGE_VIEW_TYPE_2D_ARRAY, // Texture2DArray
	VK_IMAGE_VIEW_TYPE_2D,		 // Texture2DMultisample
	VK_IMAGE_VIEW_TYPE_2D_ARRAY, // Texture2DMultisampleArray
	VK_IMAGE_VIEW_TYPE_3D,		 // Texture3D
	VK_IMAGE_VIEW_TYPE_CUBE		 // TextureCube
};
static_assert(core::enumVal(TextureType::Max) == lengthof(ImageViewTypes), "Array sizes don't match Max");

static const VkFilter TextureFilters[] = {
	VK_FILTER_LINEAR, // Linear
	VK_FILTER_NEAREST, // Nearest
	VK_FILTER_NEAREST, // NearestMipmapNearest -> nearest
	VK_FILTER_NEAREST, // NearestMipmapLinear -> nearest (mipmap mode handled separately)
	VK_FILTER_LINEAR,  // LinearMipmapNearest -> linear
	VK_FILTER_LINEAR   // LinearMipmapLinear -> linear
};
static_assert(core::enumVal(TextureFilter::Max) == lengthof(TextureFilters), "Array sizes don't match Max");

static const VkSamplerAddressMode TextureWraps[] = {
	VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,	 // ClampToEdge
	VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // ClampToBorder
	VK_SAMPLER_ADDRESS_MODE_REPEAT,			 // Repeat
	VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT	 // MirroredRepeat
};
static_assert(core::enumVal(TextureWrap::Max) == lengthof(TextureWraps), "Array sizes don't match Max");

static const VkBufferUsageFlagBits BufferUsages[] = {
	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,		  // ArrayBuffer
	VK_BUFFER_USAGE_INDEX_BUFFER_BIT,		  // IndexBuffer
	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,		  // UniformBuffer
	VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, // TransformBuffer
	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,		  // PixelBuffer
	VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,		  // ShaderStorageBuffer
	VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT		  // IndirectBuffer
};
static_assert(core::enumVal(BufferType::Max) == lengthof(BufferUsages), "Array sizes don't match Max");

static const VkMemoryPropertyFlags MemoryProperties[] = {
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,										// Static
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Dynamic
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	// Stream
};
static_assert(core::enumVal(BufferMode::Max) == lengthof(MemoryProperties), "Array sizes don't match Max");

struct Formats {
	VkFormat internalFormat;
	VkFormat format;
	VkFormat type;
	int bits;
};

static const Formats textureFormats[] = {
	{VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, 32},					// RGBA
	{VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_UNORM, 24},						// RGB
	{VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, 128}, // RGBA32F
	{VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, 96},			// RGB32F
	{VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, 64},	// RGBA16F
	{VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, 32},		// D24S8
	{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, 40},		// D32FS8
	{VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, 24},		// D24
	{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT, 32},								// D32F
	{VK_FORMAT_S8_UINT, VK_FORMAT_S8_UINT, VK_FORMAT_S8_UINT, 8},										// S8
	{VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_UINT, 32}							// RG16U
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(textureFormats), "Array sizes don't match Max");

inline VkImageAspectFlags getImageAspectFlags(TextureFormat format) {
	switch (format) {
	case TextureFormat::D24S8:
	case TextureFormat::D32FS8:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	case TextureFormat::D24:
	case TextureFormat::D32F:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case TextureFormat::S8:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

inline bool isDepthFormat(TextureFormat format) {
	return format == TextureFormat::D24S8 || format == TextureFormat::D32FS8 || format == TextureFormat::D24 ||
		   format == TextureFormat::D32F;
}

inline bool isStencilFormat(TextureFormat format) {
	return format == TextureFormat::D24S8 || format == TextureFormat::D32FS8 || format == TextureFormat::S8;
}

} // namespace _priv
} // namespace video
