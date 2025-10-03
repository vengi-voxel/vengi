/**
 * @file
 */

#pragma once

#include "video/Types.h"
#include "core/ArrayLength.h"
#include "flextGL.h"

namespace video {

namespace _priv {

static const struct Formats {
	uint8_t bits;
	GLenum internalFormat;
	GLenum dataFormat;
	GLenum dataType;
} textureFormats[] = {
	{32, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
	{24, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},
	{32, GL_RGBA32F, GL_RGBA, GL_FLOAT},
	{24, GL_RGB32F, GL_RGB, GL_FLOAT},
	{16, GL_RGBA16F, GL_RGBA, GL_FLOAT},

	{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8},
	{32, GL_DEPTH32F_STENCIL8, GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
	{32, GL_DEPTH_COMPONENT24, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8},
	{32, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT},
	{0, GL_STENCIL_INDEX8, GL_STENCIL_INDEX8, GL_STENCIL_INDEX8},
	{16, GL_RG16UI, GL_RG, GL_UNSIGNED_BYTE}
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(textureFormats), "Array sizes don't match Max");

static const GLenum TextureFormats[] {
	GL_RGBA8,
	GL_RGB8,
	GL_RGBA32F,
	GL_RGB32F,
	GL_RGBA16F,

	GL_DEPTH24_STENCIL8,
	GL_DEPTH32F_STENCIL8,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH_COMPONENT32F,
	GL_STENCIL_INDEX8,

	GL_RG16UI
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(TextureFormats), "Array sizes don't match Max");

static const GLenum ShaderTypes[] {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
	GL_COMPUTE_SHADER
};
static_assert(core::enumVal(ShaderType::Max) == lengthof(ShaderTypes), "Array sizes don't match Max");

static const GLenum MemoryBarrierTypes[] {
	0,
	GL_SHADER_IMAGE_ACCESS_BARRIER_BIT,
	GL_ALL_BARRIER_BITS
};
static_assert(core::enumVal(MemoryBarrierType::Max) == lengthof(MemoryBarrierTypes), "Array sizes don't match Max");

static const GLenum StencilOps[] {
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INCR,
	GL_INCR_WRAP,
	GL_DECR,
	GL_DECR_WRAP,
	GL_INVERT
};
static_assert(core::enumVal(StencilOp::Max) == lengthof(StencilOps), "Array sizes don't match Max");

static const GLenum FrameBufferAttachments[] {
	GL_DEPTH_STENCIL_ATTACHMENT,
	GL_DEPTH_ATTACHMENT,
	GL_STENCIL_ATTACHMENT,
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
	GL_COLOR_ATTACHMENT8,
	GL_COLOR_ATTACHMENT9,
	GL_COLOR_ATTACHMENT10,
	GL_COLOR_ATTACHMENT11,
	GL_COLOR_ATTACHMENT12,
	GL_COLOR_ATTACHMENT13,
	GL_COLOR_ATTACHMENT14,
	GL_COLOR_ATTACHMENT15
};
static_assert(core::enumVal(FrameBufferAttachment::Max) == lengthof(FrameBufferAttachments), "Array sizes don't match Max");

static const GLenum FrameBufferModes[] {
	GL_READ_FRAMEBUFFER,
	GL_DRAW_FRAMEBUFFER,
	GL_FRAMEBUFFER
};
static_assert(core::enumVal(FrameBufferMode::Max) == lengthof(FrameBufferModes), "Array sizes don't match Max");

/**
 * GL_VENDOR check - case insensitive
 */
static const char* VendorStrings[] {
	"nouveau",
	"intel",
	"nvidia",
	"amd"
};
static_assert(core::enumVal(Vendor::Max) == lengthof(VendorStrings), "Array sizes don't match Max");

static const GLenum BufferModes[] {
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_STREAM_DRAW
};
static_assert(core::enumVal(BufferMode::Max) == lengthof(BufferModes), "Array sizes don't match Max");

static const GLenum AccessModes[] {
	GL_READ_ONLY,
	GL_WRITE_ONLY,
	GL_READ_WRITE
};
static_assert(core::enumVal(AccessMode::Max) == lengthof(AccessModes), "Array sizes don't match Max");

static const GLenum BufferTypes[] {
	GL_ARRAY_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_PIXEL_UNPACK_BUFFER,
	GL_SHADER_STORAGE_BUFFER,
	GL_DRAW_INDIRECT_BUFFER
};
static_assert(core::enumVal(BufferType::Max) == lengthof(BufferTypes), "Array sizes don't match Max");

static const GLenum States[] {
	0,
	GL_STENCIL_TEST,
	GL_DEPTH_TEST,
	GL_CULL_FACE,
	GL_BLEND,
	GL_POLYGON_OFFSET_FILL,
	GL_POLYGON_OFFSET_POINT,
	GL_POLYGON_OFFSET_LINE,
	GL_SCISSOR_TEST,
	GL_MULTISAMPLE,
	GL_LINE_SMOOTH,
	GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB,
	GL_CLIP_DISTANCE0,
	GL_PRIMITIVE_RESTART,
	GL_PROGRAM_POINT_SIZE
};
static_assert(core::enumVal(State::Max) == lengthof(States), "Array sizes don't match Max");

const GLenum TextureTypes[] {
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_2D_MULTISAMPLE,
	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP
};
static_assert(core::enumVal(TextureType::Max) == lengthof(TextureTypes), "Array sizes don't match Max");

static const GLenum TextureFilters[] {
	GL_LINEAR,
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};
static_assert(core::enumVal(TextureFilter::Max) == lengthof(TextureFilters), "Array sizes don't match Max");

static const GLenum TextureWraps[] {
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
	GL_REPEAT,
	GL_MIRRORED_REPEAT
};
static_assert(core::enumVal(TextureWrap::Max) == lengthof(TextureWraps), "Array sizes don't match Max");

static const GLenum BlendModes[] {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR
};
static_assert(core::enumVal(BlendMode::Max) == lengthof(BlendModes), "Array sizes don't match Max");

static const GLenum BlendEquations[] {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};
static_assert(core::enumVal(BlendEquation::Max) == lengthof(BlendEquations), "Array sizes don't match Max");

static const GLenum CompareFuncs[] {
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS
};
static_assert(core::enumVal(CompareFunc::Max) == lengthof(CompareFuncs), "Array sizes don't match Max");

static const GLenum TextureCompareModes[] {
	GL_NONE,
	GL_COMPARE_REF_TO_TEXTURE
};
static_assert(core::enumVal(TextureCompareMode::Max) == lengthof(TextureCompareModes), "Array sizes don't match Max");

static const GLenum PolygonModes[] {
	GL_POINT,
	GL_LINE,
	GL_FILL
};
static_assert(core::enumVal(PolygonMode::Max) == lengthof(PolygonModes), "Array sizes don't match Max");

static const GLenum Faces[] {
	GL_FRONT,
	GL_BACK,
	GL_FRONT_AND_BACK
};
static_assert(core::enumVal(Face::Max) == lengthof(Faces), "Array sizes don't match Max");

static const GLenum Primitives[] {
	GL_POINTS,
	GL_LINES,
	GL_LINES_ADJACENCY,
	GL_TRIANGLES,
	GL_TRIANGLES_ADJACENCY,
	GL_LINE_STRIP,
	GL_TRIANGLE_STRIP
};
static_assert(core::enumVal(Primitive::Max) == lengthof(Primitives), "Array sizes don't match Max");

static const GLenum TextureUnits[] {
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,
	GL_TEXTURE4,
	GL_TEXTURE5,
	GL_TEXTURE6,
	GL_TEXTURE7,
	GL_TEXTURE8,
	GL_TEXTURE9,
	GL_TEXTURE10
};
static_assert(core::enumVal(TextureUnit::Max) == lengthof(TextureUnits), "Array sizes don't match Max");

static const GLenum DataTypes[] {
	GL_DOUBLE,
	GL_FLOAT,
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT
};
static_assert(core::enumVal(DataType::Max) == lengthof(DataTypes), "Array sizes don't match Max");

static const GLenum ObjectNameTypes[] = {
	GL_BUFFER,
	GL_SHADER,
	GL_PROGRAM,
	GL_VERTEX_ARRAY,
	GL_QUERY,
	GL_PROGRAM_PIPELINE,
	GL_TRANSFORM_FEEDBACK,
	GL_SAMPLER,
	GL_TEXTURE,
	GL_RENDERBUFFER,
	GL_FRAMEBUFFER
};
static_assert(core::enumVal(ObjectNameType::Max) == lengthof(ObjectNameTypes), "Array sizes don't match Max");

static const GLenum ImageFormatTypes[] = {
	GL_RGBA32F,
	GL_RGBA16F,
	GL_RG32F,
	GL_RG16F,
	GL_R11F_G11F_B10F,
	GL_R32F,
	GL_R16F,
	GL_RGBA16,
	GL_RGB10_A2,
	GL_RGBA8,
	GL_RG16,
	GL_RG8,
	GL_R16,
	GL_R8,
	GL_RGBA16_SNORM,
	GL_RGBA8_SNORM,
	GL_RG16_SNORM,
	GL_RG8_SNORM,
	GL_R16_SNORM,
	GL_R8_SNORM,
	GL_RGBA32I,
	GL_RGBA16I,
	GL_RGBA8I,
	GL_RG32I,
	GL_RG16I,
	GL_RG8I,
	GL_R32I,
	GL_R16I,
	GL_R8I,
	GL_RGBA32UI,
	GL_RGBA16UI,
	GL_RGB10_A2UI,
	GL_RGBA8UI,
	GL_RG32UI,
	GL_RG16UI,
	GL_RG8UI,
	GL_R32UI,
	GL_R16UI,
	GL_R8UI
};
static_assert((size_t)video::ImageFormat::Max == lengthof(ImageFormatTypes), "mismatch in image formats");

}

}
