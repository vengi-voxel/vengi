/**
 * @file
 */

#pragma once

#include "video/Types.h"
#include "core/Array.h"
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
	{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8},
	{32, GL_RGBA32F, GL_RGBA, GL_FLOAT}
};
static_assert(std::enum_value(TextureFormat::Max) == lengthof(textureFormats), "Array sizes don't match Max");

static GLenum ShaderTypes[] {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
#ifdef GL_COMPUTE_SHADER
	GL_COMPUTE_SHADER
#else
	0
#endif
};
static_assert(std::enum_value(ShaderType::Max) == lengthof(ShaderTypes), "Array sizes don't match Max");

static GLenum FrameBufferModes[] {
	GL_READ_FRAMEBUFFER,
	GL_DRAW_FRAMEBUFFER,
	GL_FRAMEBUFFER
};
static_assert(std::enum_value(FrameBufferMode::Max) == lengthof(FrameBufferModes), "Array sizes don't match Max");

/**
 * GL_VENDOR check - case insensitive
 */
static const char* VendorStrings[] {
	"nouveau",
	"intel",
	"nvidia",
	"amd"
};
static_assert(std::enum_value(Vendor::Max) == lengthof(VendorStrings), "Array sizes don't match Max");

static GLenum VertexBufferModes[] {
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_STREAM_DRAW
};
static_assert(std::enum_value(VertexBufferMode::Max) == lengthof(VertexBufferModes), "Array sizes don't match Max");

static GLenum AccessModes[] {
	GL_READ_ONLY,
	GL_WRITE_ONLY,
	GL_READ_WRITE
};
static_assert(std::enum_value(AccessMode::Max) == lengthof(AccessModes), "Array sizes don't match Max");

static GLenum VertexBufferTypes[] {
	GL_ARRAY_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER
};
static_assert(std::enum_value(VertexBufferType::Max) == lengthof(VertexBufferTypes), "Array sizes don't match Max");

static GLenum States[] {
	0,
	GL_DEPTH_TEST,
	GL_CULL_FACE,
	GL_BLEND,
	GL_POLYGON_OFFSET_FILL,
	GL_POLYGON_OFFSET_POINT,
	GL_POLYGON_OFFSET_LINE,
	GL_SCISSOR_TEST,
	GL_MULTISAMPLE,
	GL_LINE_SMOOTH,
	GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB
};
static_assert(std::enum_value(State::Max) == lengthof(States), "Array sizes don't match Max");

static GLenum TextureTypes[] {
	GL_TEXTURE_2D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_CUBE_MAP
};
static_assert(std::enum_value(TextureType::Max) == lengthof(TextureTypes), "Array sizes don't match Max");

static GLenum TextureFilters[] {
	GL_LINEAR,
	GL_NEAREST
};
static_assert(std::enum_value(TextureFilter::Max) == lengthof(TextureFilters), "Array sizes don't match Max");

static GLenum TextureWraps[] {
	GL_CLAMP_TO_EDGE,
	GL_REPEAT,
	GL_NONE
};
static_assert(std::enum_value(TextureWrap::Max) == lengthof(TextureWraps), "Array sizes don't match Max");

static GLenum BlendModes[] {
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
static_assert(std::enum_value(BlendMode::Max) == lengthof(BlendModes), "Array sizes don't match Max");

static GLenum BlendEquations[] {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};
static_assert(std::enum_value(BlendEquation::Max) == lengthof(BlendEquations), "Array sizes don't match Max");

static GLenum CompareFuncs[] {
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS
};
static_assert(std::enum_value(CompareFunc::Max) == lengthof(CompareFuncs), "Array sizes don't match Max");

static GLenum PolygonModes[] {
	GL_POINT,
	GL_LINE,
	GL_FILL
};
static_assert(std::enum_value(PolygonMode::Max) == lengthof(PolygonModes), "Array sizes don't match Max");

static GLenum Faces[] {
	GL_FRONT,
	GL_BACK,
	GL_FRONT_AND_BACK
};
static_assert(std::enum_value(Face::Max) == lengthof(Faces), "Array sizes don't match Max");

static GLenum Primitives[] {
	GL_POINTS,
	GL_LINES,
	GL_LINES_ADJACENCY,
	GL_TRIANGLES,
	GL_TRIANGLES_ADJACENCY,
	GL_LINE_STRIP,
	GL_TRIANGLE_STRIP
};
static_assert(std::enum_value(Primitive::Max) == lengthof(Primitives), "Array sizes don't match Max");

static GLenum TransformFeedbackCaptureModes[] = {
	GL_INTERLEAVED_ATTRIBS,
	GL_SEPARATE_ATTRIBS
};
static_assert(std::enum_value(TransformFeedbackCaptureMode::Max) == lengthof(TransformFeedbackCaptureModes), "Array sizes don't match Max");

static GLenum TextureUnits[] {
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,
	GL_TEXTURE4,
	GL_TEXTURE5,
	GL_TEXTURE6
};
static_assert(std::enum_value(TextureUnit::Max) == lengthof(TextureUnits), "Array sizes don't match Max");

static GLenum DataTypes[] {
	GL_DOUBLE,
	GL_FLOAT,
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT
};
static_assert(std::enum_value(DataType::Max) == lengthof(DataTypes), "Array sizes don't match Max");

static GLenum ImageFormatTypes[] = {
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
