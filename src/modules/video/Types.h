#pragma once

#include "GLFunc.h"

namespace video {

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

enum class Limit {
	MaxTextureSize,
	MaxCubeMapTextureSize,
	MaxViewPortWidth,
	MaxDrawBuffers,
	MaxViewPortHeight,
	MaxVertexAttribs,
	MaxVertexUniformComponents,
	MaxVaryingComponents,
	MaxCombinedTextureImageUnits,
	MaxVertexTextureImageUnits,
	MaxElementIndices,
	MaxElementVertices,
	MaxFragmentInputComponents,
	MaxFragmentUniformComponents,

	Max
};

enum class Feature {
	TextureCompressionDXT,
	TextureCompressionPVRTC,
	TextureCompressionETC2,
	TextureCompressionATC,
	TextureFloat,
	TextureHalfFloat,
	InstancedArrays,
	DebugOutput,

	Max
};

}
