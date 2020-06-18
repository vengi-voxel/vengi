/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include <stdint.h>
#include "gl/GLTypes.h"

/**
 * Rendering
 */
namespace video {

enum class TextureType {
	Texture1D,
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube,

	Max
};

enum class TextureFilter {
	Linear,
	Nearest,

	Max
};

enum class TextureWrap {
	ClampToEdge,
	ClampToBorder,
	Repeat,
	MirroredRepeat,

	Max
};

enum class TextureCompareMode {
	None,
	RefToTexture,

	Max
};

enum class ClearFlag {
	None,
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

enum class StencilOp {
	/** The current value is kept. */
	Keep,
	/** The stencil value is set to 0. */
	Zero,
	/** The stencil value is set to the reference value. */
	Replace,
	/** The stencil value is increased by 1 if it is lower than the maximum value. */
	Incr,
	/** Same as @c Incr, with the exception that the value is set to 0 if the maximum value is exceeded. */
	IncrWrap,
	/** The stencil value is decreased by 1 if it is higher than 0. */
	Decr,
	/** Same as @c Decr, with the exception that the value is set to the maximum value if the current value
	 * is 0 (the stencil buffer stores unsigned integers). */
	DecrWrap,
	/** A bitwise invert is applied to the value. */
	Invert,

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
	Amd,

	Max
};

enum class State {
	DepthMask,
	StencilTest,
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
	ClipDistance,

	Max
};

enum class AccessMode {
	Read,
	Write,
	ReadWrite,

	Max
};

enum class BufferType {
	ArrayBuffer,
	IndexBuffer,
	UniformBuffer,
	TransformBuffer,
	PixelBuffer,
	ShaderStorageBuffer,
	IndirectBuffer,

	Max
};

enum class BufferMode {
	Static,
	Dynamic,
	Stream,

	Max
};

enum class TransformFeedbackCaptureMode {
	Interleaved,
	Separate,

	Max
};

enum class Primitive {
	Points,
	Lines,
	LinesAdjacency,
	Triangles,
	TrianglesAdjacency,
	LineStrip,
	TriangleStrip,

	Max
};

enum class FrameBufferMode {
	Read,
	Draw,
	Default,

	Max
};

enum class FrameBufferAttachment {
	DepthStencil,
	Depth,
	Stencil,
	Color0,
	Color1,
	Color2,
	Color3,
	Color4,
	Color5,
	Color6,
	Color7,
	Color8,
	Color9,
	Color10,
	Color11,
	Color12,
	Color13,
	Color14,
	Color15,

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
	Six,
	Seven,
	Eight,
	Nine,

	// don't interfere with any other bound texture when we are uploading
	Upload,

	Max
};

enum class ImageFormat {
	// floating point
	RGBA32F,
	RGBA16F,
	RG32F,
	RG16F,
	R11F_G11F_B10F,
	R32F,
	R16F,
	RGBA16,
	RGB10_A2,
	RGBA8,
	RG16,
	RG8,
	R16,
	R8,
	RGBA16_SNORM,
	RGBA8_SNORM,
	RG16_SNORM,
	RG8_SNORM,
	R16_SNORM,
	R8_SNORM,

	// signed integer
	RGBA32I,
	RGBA16I,
	RGBA8I,
	RG32I,
	RG16I,
	RG8I,
	R32I,
	R16I,
	R8I,

	// unsigned integer
	RGBA32UI,
	RGBA16UI,
	RGB10_A2UI,
	RGBA8UI,
	RG32UI,
	RG16UI,
	RG8UI,
	R32UI,
	R16UI,
	R8UI,

	Max
};

enum class TextureFormat {
	RGBA,
	RGB,
	RGBA32F,
	RGB32F,

	D24S8,
	D32FS8,
	D24,
	D32F,
	S8,

	RG16U,

	Max
};

enum class Spec {
	UniformBufferAlignment,
	ShaderStorageBufferOffsetAlignment,

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
	MaxComputeWorkGroupSizeX,
	MaxComputeWorkGroupSizeY,
	MaxComputeWorkGroupSizeZ,
	MaxComputeWorkGroupCountX,
	MaxComputeWorkGroupCountY,
	MaxComputeWorkGroupCountZ,
	MaxComputeWorkGroupInvocations,
	MaxUniformBufferSize,
	MaxUniformBufferBindings,
	MaxShaderStorageBufferSize,

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
	ComputeShaders,
	TransformFeedback,
	ShaderStorageBufferObject,

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

/**
 * Vertex buffer shader attributes
 */
struct Attribute {
	/** shader attribute index */
	int32_t location = -1;
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
	/** if this is true, the values are not converted to float, but are kept as integers */
	bool typeIsInt = false;
};

}
