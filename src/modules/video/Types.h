#pragma once

#include "gl/GLTypes.h"

namespace video {

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
