#pragma once

#include "GLFunc.h"
#include "core/Common.h"
#include <cstdint>

namespace video {

enum class ClearFlag : GLbitfield {
	Color = GL_COLOR_BUFFER_BIT,
	Depth = GL_DEPTH_BUFFER_BIT
};
CORE_ENUM_BIT_OPERATIONS(ClearFlag)

enum class Primitive {
	Points = GL_POINTS,
	Lines = GL_LINES,
	Triangles = GL_TRIANGLES
};

enum class PolygonMode {
	Points = GL_POINT,
	WireFrame = GL_LINE,
	Solid = GL_FILL
};

enum class TextureUnit : int32_t {
	Zero  = GL_TEXTURE0,
	One   = GL_TEXTURE1,
	Two   = GL_TEXTURE2,
	Three = GL_TEXTURE3,
	Four  = GL_TEXTURE4,
	Five  = GL_TEXTURE5,

	// don't interfer with any other bound texture when we are uploading
	Upload = GL_TEXTURE10
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
	OneMinusDestinationColor = GL_ONE_MINUS_DST_COLOR
};

}
