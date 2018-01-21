/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <stdint.h>
#include <unordered_map>
#include "gl/GLTypes.h"

namespace video {

enum class TextureType {
	Texture2D,
	Texture2DArray,
	TextureCube,

	Max
};

enum class TextureWrap {
	ClampToEdge,
	Repeat,
	None,

	Max
};

enum class ClearFlag {
	Color,
	Depth,
	Stencil,

	Max
};
CORE_ENUM_BIT_OPERATIONS(ClearFlag)

enum class Face {
	Front,
	Back,
	FrontAndBack,

	Max
};

enum class PolygonMode {
	Points,
	WireFrame,
	Solid,

	Max
};

enum class CompareFunc {
	Never,
	Less,
	Equal,
	// Accept fragment if it closer to the camera than the former one
	LessEqual,
	Greater,
	NotEqual,
	GreatorEqual,
	Always,

	Max
};

enum class BlendMode {
	Zero,
	One,
	SourceColor,
	OneMinusSourceColor,
	SourceAlpha,
	OneMinusSourceAlpha,
	DestinationAlpha,
	OneMinusDestinationAlpha,
	DestinationColor,
	OneMinusDestinationColor,

	Max
};

enum class BlendEquation {
	Add,
	Subtract,
	ReverseSubtract,
	Minimum,
	Maximum,

	Max
};

enum class Vendor {
	Nouveau,
	Intel,
	Nvidia,

	Max
};

enum class State {
	DepthMask,
	DepthTest,
	// Cull triangles whose normal is not towards the camera
	CullFace,
	Blend,
	PolygonOffsetFill,
	PolygonOffsetPoint,
	PolygonOffsetLine,
	Scissor,
	MultiSample,
	LineSmooth,
	DebugOutput,

	Max
};

enum class VertexBufferAccessMode {
	Read,
	Write,
	ReadWrite,

	Max
};

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

enum class Primitive {
	Points,
	Lines,
	Triangles,

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
	Compute,

	Max
};

enum class TextureUnit {
	Zero,
	One,
	Two,
	Three,
	Four,
	Five,

	// don't interfere with any other bound texture when we are uploading
	Upload,

	Max
};

enum class TextureFormat {
	RGBA,
	RGB,
	D24S8,

	Max
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
	DirectStateAccess,
	BufferStorage,
	MultiDrawIndirect,

	Max
};

enum class DataType {
	Double,
	Float,
	UnsignedByte,
	Byte,
	UnsignedShort,
	Short,
	UnsignedInt,
	Int,

	Max
};

enum GBufferTextureType {
	GBUFFER_TEXTURE_TYPE_POSITION, GBUFFER_TEXTURE_TYPE_DIFFUSE, GBUFFER_TEXTURE_TYPE_NORMAL, GBUFFER_NUM_TEXTURES
};

enum class DebugSeverity {
	High,
	Medium,
	Low,
	Max
};

struct Uniform {
	int location;
	bool block;
};
typedef std::unordered_map<std::string, Uniform> ShaderUniforms;

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
	 * will use the second, etc.
	 */
	uint8_t divisor = 0;
	bool normalized = false;
	/** use glVertexAttribPointer or glVertexAttribIPointer for uploading */
	bool typeIsInt = false;
};

typedef std::unordered_map<std::string, int> ShaderAttributes;

}
