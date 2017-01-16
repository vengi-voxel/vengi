#pragma once

namespace video {

enum class DepthBufferMode {
	// stores -1..1 window-space depth values
	RGBA,
	// stores 0..1 window-space depth values
	DEPTH,
	DEPTH_CMP
};

enum class TextureFormat {
	RGBA,
	RGB,
	D24S8
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

#include "gl/GLTypes.h"
