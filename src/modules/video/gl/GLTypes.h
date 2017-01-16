#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include <cstdint>

namespace video {

using Id = GLuint;
constexpr Id InvalidId = (Id)0;

enum class FrameBufferMode {
	Read = GL_READ_FRAMEBUFFER,
	Draw = GL_DRAW_FRAMEBUFFER,
	Default = GL_FRAMEBUFFER,
	Max
};

enum class VertexBufferMode {
	Static = GL_STATIC_DRAW,
	Dynamic = GL_DYNAMIC_DRAW,
	Stream = GL_STREAM_DRAW
};

enum class VertexBufferType {
#ifdef GL_DRAW_INDIRECT_BUFFER
	DrawIndirectBuffer = GL_DRAW_INDIRECT_BUFFER,
#endif
	ArrayBuffer = GL_ARRAY_BUFFER,
	IndexBuffer = GL_ELEMENT_ARRAY_BUFFER,
	UniformBuffer = GL_UNIFORM_BUFFER,
	Max
};

enum class TextureType {
	Texture2D = GL_TEXTURE_2D,
	Texture2DArray = GL_TEXTURE_2D_ARRAY,
	TextureCube = GL_TEXTURE_CUBE_MAP
};

enum class TextureWrap {
	ClampToEdge = GL_CLAMP_TO_EDGE,
	Repeat = GL_REPEAT
};

enum class BufferLockMode {
	Normal = GL_MAP_WRITE_BIT,
	Discard = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
	Read = GL_MAP_READ_BIT,

	Max
};

enum class State {
	DepthMask = 0,
	DepthTest = GL_DEPTH_TEST,
	// Cull triangles whose normal is not towards the camera
	CullFace = GL_CULL_FACE,
	Blend = GL_BLEND,
	PolygonOffsetFill = GL_POLYGON_OFFSET_FILL,
	PolygonOffsetPoint = GL_POLYGON_OFFSET_POINT,
	PolygonOffsetLine = GL_POLYGON_OFFSET_LINE,
	Scissor = GL_SCISSOR_TEST,
	MultiSample = GL_MULTISAMPLE,

	Max
};

enum class CompareFunc {
	Never = GL_NEVER,
	Less = GL_LESS,
	Equal = GL_EQUAL,
	// Accept fragment if it closer to the camera than the former one
	LessEqual = GL_LEQUAL,
	Greater = GL_GREATER,
	NotEqual = GL_NOTEQUAL,
	GreatorEqual = GL_GEQUAL,
	Always = GL_ALWAYS,

	Max
};

enum class Face {
	Front = GL_FRONT,
	Back = GL_BACK,
	FrontAndBack = GL_FRONT_AND_BACK,

	Max
};

enum class ClearFlag : GLbitfield {
	Color = GL_COLOR_BUFFER_BIT,
	Depth = GL_DEPTH_BUFFER_BIT,

	Max
};
CORE_ENUM_BIT_OPERATIONS(ClearFlag)

enum class Primitive {
	Points = GL_POINTS,
	Lines = GL_LINES,
	Triangles = GL_TRIANGLES,

	Max
};

enum class PolygonMode {
	Points = GL_POINT,
	WireFrame = GL_LINE,
	Solid = GL_FILL,

	Max
};

enum class TextureUnit : int32_t {
	Zero  = GL_TEXTURE0,
	One   = GL_TEXTURE1,
	Two   = GL_TEXTURE2,
	Three = GL_TEXTURE3,
	Four  = GL_TEXTURE4,
	Five  = GL_TEXTURE5,

	// don't interfer with any other bound texture when we are uploading
	Upload = GL_TEXTURE10,

	Max
};

enum class BlendMode {
	Zero = GL_ZERO,
	One = GL_ONE,
	SourceColor = GL_SRC_COLOR,
	OneMinusSourceColor = GL_ONE_MINUS_SRC_COLOR,
	SourceAlpha = GL_SRC_ALPHA,
	OneMinusSourceAlpha = GL_ONE_MINUS_SRC_ALPHA,
	DestinationAlpha = GL_DST_ALPHA,
	OneMinusDestinationAlpha = GL_ONE_MINUS_DST_ALPHA,
	DestinationColor = GL_DST_COLOR,
	OneMinusDestinationColor = GL_ONE_MINUS_DST_COLOR,

	Max
};

}
