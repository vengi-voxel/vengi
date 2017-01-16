#pragma once

#include "GLFunc.h"
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

}
