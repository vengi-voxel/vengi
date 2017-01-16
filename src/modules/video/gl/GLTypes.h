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
