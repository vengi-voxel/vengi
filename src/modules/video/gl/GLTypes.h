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

enum class BufferLockMode {
	Normal = GL_MAP_WRITE_BIT,
	Discard = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
	Read = GL_MAP_READ_BIT,

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

}
