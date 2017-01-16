#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include <cstdint>

namespace video {

using Id = GLuint;
constexpr Id InvalidId = (Id)0;

enum class DataType {
	Double = GL_DOUBLE,
	Float = GL_FLOAT,
	UnsignedByte = GL_UNSIGNED_BYTE,
	Byte = GL_BYTE,
	UnsignedShort = GL_UNSIGNED_SHORT,
	Short = GL_SHORT,
	UnsignedInt = GL_UNSIGNED_INT,
	Int = GL_INT,

	Max
};

enum class TextureWrap {
	ClampToEdge = GL_CLAMP_TO_EDGE,
	Repeat = GL_REPEAT,

	Max
};

enum class BufferLockMode {
	Normal = GL_MAP_WRITE_BIT,
	Discard = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
	Read = GL_MAP_READ_BIT,

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

	// don't interfere with any other bound texture when we are uploading
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
