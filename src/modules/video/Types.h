#pragma once

#include <cstdint>

namespace video {

enum class VertexBufferType {
	ArrayBuffer,
	IndexBuffer,
	UniformBuffer,

	Max
};

enum class VertexBufferMode {
	Static,
	Dynamic,
	Stream,

	Max
};

enum class DepthBufferMode {
	// stores -1..1 window-space depth values
	RGBA,
	// stores 0..1 window-space depth values
	DEPTH,
	DEPTH_CMP
};

enum class FrameBufferMode {
	Read,
	Draw,
	Default,

	Max
};

enum class ShaderType {
	Vertex,
	Fragment,
	Geometry,

	Max
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

// TODO: make private
#include "gl/GLTypes.h"

namespace video {

/**
 * Vertex buffer shader attributes
 */
struct Attribute {
	/** shader attribute index */
	int32_t index = -1;
	/** The internal buffer index that was returned by @c create() */
	int32_t bufferIndex = -1;
	/** The size behind your attribute (not sizeof but lengthof). */
	int size = 0;
	/** the amount of bytes between each attribute instance */
	int stride = 0;
	/** the offset of the buffer to start reading from */
	intptr_t offset = 0;
	/** The data type behind your attribute - also see @c typeIsInt */
	video::DataType type = video::DataType::Float;
	/**
	 * The rate by which the attribute advances during instanced rendering. It basically means the number of
	 * times the entire set of vertices is rendered before the attribute is updated from the buffer. By default,
	 * the divisor is zero. This causes regular vertex attributes to be updated from vertex to vertex. If the divisor
	 * is 10 it means that the first 10 instances will use the first piece of data from the buffer, the next 10 instances
	 * will use the second, etc. We want to have a dedicated WVP matrix for each instance so we use a divisor of 1.
	 */
	uint8_t divisor = 0;
	bool normalized = false;
	/** use glVertexAttribPointer or glVertexAttribIPointer for uploading */
	bool typeIsInt = false;
};

}
