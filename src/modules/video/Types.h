/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "engine-config.h"
#include <stdint.h>
#if USE_GL_RENDERER
#include "gl/GLTypes.h"
#elif USE_VK_RENDERER
#include "vk/VKTypes.h"
#else
#error "renderer not supported"
#endif

/**
 * Rendering
 */
namespace video {

enum class TextureType {
	Texture1D,
	Texture2D,
	Texture2DArray,
	Texture2DMultisample,
	Texture2DMultisampleArray,
	Texture3D,
	TextureCube,

	Max
};

enum class TextureFilter {
	Linear,
	Nearest,

	// Mipmap-aware minification filters
	NearestMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapNearest,
	LinearMipmapLinear,

	Max
};

enum class TextureWrap {
	ClampToEdge,
	ClampToBorder,
	Repeat,
	MirroredRepeat,

	Max
};

enum class QueryType {
	SamplesPassed,
	AnySamplesPassed,
	TimeElapsed,
	Timestamp,
	PrimitivesGenerated,
	TransformFeedbackPrimitivesWritten,

	Max
};

enum class ClearFlag {
	None = 0,
	Color = 1,
	Depth = 2,
	Stencil = 4,

	Max = 5
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

/**
 * @brief Specifies the texture comparison mode for currently bound depth textures. That is, a texture whose internal
 * format is a depth TextureFormat)
 */
enum class TextureCompareMode {
	/**
	 * Specifies that the red channel should be assigned the appropriate value from the currently bound depth texture.
	 */
	None,
	/**
	 * Specifies that the interpolated and clamped texture coordinate should be compared to the value in the currently
	 * bound depth texture.
	 * @sa CompareFunc for details of how the comparison is evaluated. The result of the comparison is assigned to the
	 * red channel.
	 */
	RefToTexture,

	Max
};

/**
 * Where r is the current interpolated texture coordinate, and DV is the depth texture value sampled from the
 * currently bound depth texture. The result is assigned to the red channel.
 *
 * @sa TextureCompareMode::RefToTexture
 */
enum class CompareFunc {
	/**
	 * @code result = 0.0 @endcode
	 */
	Never,
	/**
	 * @code result = 1.0 0.0; r < DV r >= DV @endcode
	 */
	Less,
	/**
	 * @code result = 1.0 0.0; r = DV r != DV @endcode
	 */
	Equal,
	/**
	 * Accept fragment if it is closer to the camera than the former one
	 * @code result = 1.0 0.0; r <= DV r > DV @endcode
	 */
	LessEqual,
	/**
	 * @code result = 1.0 0.0; r > DV r <= DV @endcode
	 */
	Greater,
	/**
	 * @code result = 1.0 0.0; r != DV r = DV @endcode
	 */
	NotEqual,
	/**
	 * @code result = 1.0 0.0; r >= DV r < DV @endcode
	 */
	GreaterOrEqual,
	/**
	 * @code result = 1.0 @endcode
	 */
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

// https://www.khronos.org/opengl/wiki/Blending#Blend_Equations
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
	DepthMask, // The depth buffer write can be masked, thus preventing the depth buffer from being updated. This useful
			   // for implementing transparency.
	StencilTest, // The Stencil Test is a per-sample operation performed after the Fragment Shader. The fragment's
				 // stencil value is tested against the value in the current stencil buffer; if the test fails, the
				 // fragment is culled.
	DepthTest, // The Depth Test is a per-sample processing operation performed after the Fragment Shader (and sometimes
			   // before). The Fragment's output depth value may be tested against the depth of the sample being written
			   // to. If the test fails, the fragment is discarded. If the test passes, the depth buffer will be updated
			   // with the fragment's output depth, unless a subsequent per-sample operation prevents it (such as
			   // turning off depth writes).
	CullFace,  // Cull triangles whose normal is not towards the camera
	Blend,
	PolygonOffsetFill,
	PolygonOffsetPoint,
	PolygonOffsetLine,
	Scissor, // The Scissor Test is a Per-Sample Processing operation that discards Fragments that fall outside of a
			 // certain rectangular portion of the screen.
	MultiSample,
	LineSmooth,
	DebugOutput,
	ClipDistance,
	PrimitiveRestart,
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPointSize.xhtml
	ProgramPointSize,

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
	/**
	 * The data store contents will be modified once and used many times.
	 * Use for static VBOs, IBOs and UBOs.
	 */
	Static,
	/**
	 * The data store contents will be modified repeatedly and used many times.
	 * Use for dynamic VBOs, IBOs and UBOs that are updated frequently.
	 */
	Dynamic,
	/**
	 * Use for streaming VBOs, IBOs and UBOs that are updated every frame.
	 */
	Stream,

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

enum class MemoryBarrierType {
	None,
	ShaderImageAccess,
	All,

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
	RGBA16F,

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
	MaxSamples,
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
	MaxLabelLength,
	MaxAnisotropy,
	MaxLodBias,

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

enum class ObjectNameType {
	Buffer,
	Shader,
	Program,
	VertexArray,
	Query,
	ProgramPipeline,
	TransformFeedback,
	Sampler,
	Texture,
	Renderbuffer,
	Framebuffer,

	Max
};

enum GBufferTextureType {
	GBUFFER_TEXTURE_TYPE_POSITION,
	GBUFFER_TEXTURE_TYPE_DIFFUSE,
	GBUFFER_TEXTURE_TYPE_NORMAL,
	GBUFFER_NUM_TEXTURES
};

enum class DebugSeverity {
	None,
	High,
	Medium,
	Low,
	Max
};

struct Uniform {
	int32_t location = -1;
	bool block = false;
	int32_t blockIndex = -1;
	int32_t blockBinding = -1;
	int32_t size = -1;
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
	 * is 10 it means that the first 10 instances will use the first piece of data from the buffer, the next 10
	 * instances will use the second, etc.
	 */
	uint8_t divisor = 0;
	bool normalized = false;
	/** if this is true, the values are not converted to float, but are kept as integers */
	bool typeIsInt = false;
};

} // namespace video
