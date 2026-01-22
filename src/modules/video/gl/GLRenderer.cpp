/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "GLTypes.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/sdl/SDLSystem.h"
#include "engine-config.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "video/Trace.h"
#include "video/Types.h"
#include "video/gl/GLVersion.h"
#include "video/gl/flextGL.h"
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#ifdef TRACY_ENABLE
#include "core/tracy/public/tracy/TracyOpenGL.hpp"
#endif

namespace video {

#ifndef MAX_SHADER_VAR_NAME
#define MAX_SHADER_VAR_NAME 128
#endif

#define SANITY_CHECKS_GL 0

namespace _priv {

struct GLState : public RendererState {
	GLVersion glVersion{0, 0};
};

static const struct Formats {
	uint8_t bits;
	GLenum internalFormat;
	GLenum dataFormat;
	GLenum dataType;
} textureFormats[] = {
	{32, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
	{24, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},
	{32, GL_RGBA32F, GL_RGBA, GL_FLOAT},
	{24, GL_RGB32F, GL_RGB, GL_FLOAT},
	{16, GL_RGBA16F, GL_RGBA, GL_FLOAT},

	{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8},
	{32, GL_DEPTH32F_STENCIL8, GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
	{32, GL_DEPTH_COMPONENT24, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8},
	{32, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT},
	{0, GL_STENCIL_INDEX8, GL_STENCIL_INDEX8, GL_STENCIL_INDEX8},
	{16, GL_RG16UI, GL_RG, GL_UNSIGNED_BYTE}
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(textureFormats), "Array sizes don't match Max");

static const GLenum TextureFormats[] {
	GL_RGBA8,
	GL_RGB8,
	GL_RGBA32F,
	GL_RGB32F,
	GL_RGBA16F,

	GL_DEPTH24_STENCIL8,
	GL_DEPTH32F_STENCIL8,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH_COMPONENT32F,
	GL_STENCIL_INDEX8,

	GL_RG16UI
};
static_assert(core::enumVal(TextureFormat::Max) == lengthof(TextureFormats), "Array sizes don't match Max");

static const GLenum ShaderTypes[] {
	GL_VERTEX_SHADER,
	GL_FRAGMENT_SHADER,
	GL_GEOMETRY_SHADER,
	GL_COMPUTE_SHADER
};
static_assert(core::enumVal(ShaderType::Max) == lengthof(ShaderTypes), "Array sizes don't match Max");

static const GLenum MemoryBarrierTypes[] {
	0,
	GL_SHADER_IMAGE_ACCESS_BARRIER_BIT,
	GL_ALL_BARRIER_BITS
};
static_assert(core::enumVal(MemoryBarrierType::Max) == lengthof(MemoryBarrierTypes), "Array sizes don't match Max");

static const GLenum StencilOps[] {
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INCR,
	GL_INCR_WRAP,
	GL_DECR,
	GL_DECR_WRAP,
	GL_INVERT
};
static_assert(core::enumVal(StencilOp::Max) == lengthof(StencilOps), "Array sizes don't match Max");

static const GLenum FrameBufferAttachments[] {
	GL_DEPTH_STENCIL_ATTACHMENT,
	GL_DEPTH_ATTACHMENT,
	GL_STENCIL_ATTACHMENT,
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
	GL_COLOR_ATTACHMENT8,
	GL_COLOR_ATTACHMENT9,
	GL_COLOR_ATTACHMENT10,
	GL_COLOR_ATTACHMENT11,
	GL_COLOR_ATTACHMENT12,
	GL_COLOR_ATTACHMENT13,
	GL_COLOR_ATTACHMENT14,
	GL_COLOR_ATTACHMENT15
};
static_assert(core::enumVal(FrameBufferAttachment::Max) == lengthof(FrameBufferAttachments), "Array sizes don't match Max");

static const GLenum FrameBufferModes[] {
	GL_READ_FRAMEBUFFER,
	GL_DRAW_FRAMEBUFFER,
	GL_FRAMEBUFFER
};
static_assert(core::enumVal(FrameBufferMode::Max) == lengthof(FrameBufferModes), "Array sizes don't match Max");

/**
 * GL_VENDOR check - case insensitive
 */
static const char* VendorStrings[] {
	"nouveau",
	"intel",
	"nvidia",
	"amd"
};
static_assert(core::enumVal(Vendor::Max) == lengthof(VendorStrings), "Array sizes don't match Max");

static const GLenum BufferModes[] {
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
	GL_STREAM_DRAW
};
static_assert(core::enumVal(BufferMode::Max) == lengthof(BufferModes), "Array sizes don't match Max");

static const GLenum AccessModes[] {
	GL_READ_ONLY,
	GL_WRITE_ONLY,
	GL_READ_WRITE
};
static_assert(core::enumVal(AccessMode::Max) == lengthof(AccessModes), "Array sizes don't match Max");

static const GLenum BufferTypes[] {
	GL_ARRAY_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_PIXEL_UNPACK_BUFFER,
	GL_SHADER_STORAGE_BUFFER,
	GL_DRAW_INDIRECT_BUFFER
};
static_assert(core::enumVal(BufferType::Max) == lengthof(BufferTypes), "Array sizes don't match Max");

static const GLenum States[] {
	0,
	GL_STENCIL_TEST,
	GL_DEPTH_TEST,
	GL_CULL_FACE,
	GL_BLEND,
	GL_POLYGON_OFFSET_FILL,
	GL_POLYGON_OFFSET_POINT,
	GL_POLYGON_OFFSET_LINE,
	GL_SCISSOR_TEST,
	GL_MULTISAMPLE,
	GL_LINE_SMOOTH,
	GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB,
	GL_CLIP_DISTANCE0,
	GL_PRIMITIVE_RESTART,
	GL_PROGRAM_POINT_SIZE
};
static_assert(core::enumVal(State::Max) == lengthof(States), "Array sizes don't match Max");

const GLenum TextureTypes[] {
	GL_TEXTURE_1D,
	GL_TEXTURE_2D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_2D_MULTISAMPLE,
	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
	GL_TEXTURE_3D,
	GL_TEXTURE_CUBE_MAP
};
static_assert(core::enumVal(TextureType::Max) == lengthof(TextureTypes), "Array sizes don't match Max");

static const GLenum TextureFilters[] {
	GL_LINEAR,
	GL_NEAREST,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR
};
static_assert(core::enumVal(TextureFilter::Max) == lengthof(TextureFilters), "Array sizes don't match Max");

static const GLenum TextureWraps[] {
	GL_CLAMP_TO_EDGE,
	GL_CLAMP_TO_BORDER,
	GL_REPEAT,
	GL_MIRRORED_REPEAT
};
static_assert(core::enumVal(TextureWrap::Max) == lengthof(TextureWraps), "Array sizes don't match Max");

static const GLenum BlendModes[] {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR
};
static_assert(core::enumVal(BlendMode::Max) == lengthof(BlendModes), "Array sizes don't match Max");

static const GLenum BlendEquations[] {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};
static_assert(core::enumVal(BlendEquation::Max) == lengthof(BlendEquations), "Array sizes don't match Max");

static const GLenum CompareFuncs[] {
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS
};
static_assert(core::enumVal(CompareFunc::Max) == lengthof(CompareFuncs), "Array sizes don't match Max");

static const GLenum TextureCompareModes[] {
	GL_NONE,
	GL_COMPARE_REF_TO_TEXTURE
};
static_assert(core::enumVal(TextureCompareMode::Max) == lengthof(TextureCompareModes), "Array sizes don't match Max");

static const GLenum PolygonModes[] {
	GL_POINT,
	GL_LINE,
	GL_FILL
};
static_assert(core::enumVal(PolygonMode::Max) == lengthof(PolygonModes), "Array sizes don't match Max");

static const GLenum Faces[] {
	GL_FRONT,
	GL_BACK,
	GL_FRONT_AND_BACK
};
static_assert(core::enumVal(Face::Max) == lengthof(Faces), "Array sizes don't match Max");

static const GLenum Primitives[] {
	GL_POINTS,
	GL_LINES,
	GL_LINES_ADJACENCY,
	GL_TRIANGLES,
	GL_TRIANGLES_ADJACENCY,
	GL_LINE_STRIP,
	GL_TRIANGLE_STRIP
};
static_assert(core::enumVal(Primitive::Max) == lengthof(Primitives), "Array sizes don't match Max");

static const GLenum TextureUnits[] {
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,
	GL_TEXTURE4,
	GL_TEXTURE5,
	GL_TEXTURE6,
	GL_TEXTURE7,
	GL_TEXTURE8,
	GL_TEXTURE9,
	GL_TEXTURE10
};
static_assert(core::enumVal(TextureUnit::Max) == lengthof(TextureUnits), "Array sizes don't match Max");

static const GLenum DataTypes[] {
	GL_DOUBLE,
	GL_FLOAT,
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT
};
static_assert(core::enumVal(DataType::Max) == lengthof(DataTypes), "Array sizes don't match Max");

static const GLenum ObjectNameTypes[] = {
	GL_BUFFER,
	GL_SHADER,
	GL_PROGRAM,
	GL_VERTEX_ARRAY,
	GL_QUERY,
	GL_PROGRAM_PIPELINE,
	GL_TRANSFORM_FEEDBACK,
	GL_SAMPLER,
	GL_TEXTURE,
	GL_RENDERBUFFER,
	GL_FRAMEBUFFER
};
static_assert(core::enumVal(ObjectNameType::Max) == lengthof(ObjectNameTypes), "Array sizes don't match Max");

static const GLenum ImageFormatTypes[] = {
	GL_RGBA32F,
	GL_RGBA16F,
	GL_RG32F,
	GL_RG16F,
	GL_R11F_G11F_B10F,
	GL_R32F,
	GL_R16F,
	GL_RGBA16,
	GL_RGB10_A2,
	GL_RGBA8,
	GL_RG16,
	GL_RG8,
	GL_R16,
	GL_R8,
	GL_RGBA16_SNORM,
	GL_RGBA8_SNORM,
	GL_RG16_SNORM,
	GL_RG8_SNORM,
	GL_R16_SNORM,
	GL_R8_SNORM,
	GL_RGBA32I,
	GL_RGBA16I,
	GL_RGBA8I,
	GL_RG32I,
	GL_RG16I,
	GL_RG8I,
	GL_R32I,
	GL_R16I,
	GL_R8I,
	GL_RGBA32UI,
	GL_RGBA16UI,
	GL_RGB10_A2UI,
	GL_RGBA8UI,
	GL_RG32UI,
	GL_RG16UI,
	GL_RG8UI,
	GL_R32UI,
	GL_R16UI,
	GL_R8UI
};
static_assert((size_t)video::ImageFormat::Max == lengthof(ImageFormatTypes), "mismatch in image formats");

static int _recompileErrors = 0;

#if defined(_WIN32) || defined(__CYGWIN__)
void __stdcall
#else
void
#endif
debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (id == 131218) {
		++_recompileErrors;
		if (_recompileErrors <= 10) {
			return;
		}
		_recompileErrors = 0;
	} else if (id == 131185) {
		// ignore hints that GL_STATIC_DRAW is used...
		return;
	}
	void (*log)(const char* msg, ...);
	const char* sourceStr;
	switch (source) {
	case GL_DEBUG_SOURCE_API_ARB:
		sourceStr = "api";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
		sourceStr = "window";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
		sourceStr = "third party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB:
		sourceStr = "app";
		break;
	case GL_DEBUG_SOURCE_OTHER_ARB:
		sourceStr = "other";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
		sourceStr = "shader";
		break;
	default:
		sourceStr = "unknown";
		break;
	}
	const char* typeStr;
	switch (type) {
	case GL_DEBUG_TYPE_ERROR_ARB:
		typeStr = "ERROR";
		log = Log::error;
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
		typeStr = "DEPRECATED_BEHAVIOR";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		typeStr = "UNDEFINED_BEHAVIOR";
		log = Log::error;
		break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:
		typeStr = "PORTABILITY";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:
		typeStr = "PERFORMANCE";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_OTHER_ARB:
		typeStr = "OTHER";
		log = Log::info;
		break;
	default:
		typeStr = "<unknown>";
		log = Log::debug;
		break;
	}
	const char* sevStr;
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW_ARB:
		sevStr = "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		sevStr = "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH_ARB:
		sevStr = "HIGH";
		log = Log::error;
		break;
	default:
		sevStr = "<unknown>";
		break;
	}
	//core_assert_msg(type == GL_DEBUG_TYPE_OTHER_ARB, "GL msg type: %s, src: %s, id: %d, severity: %s\nmsg: %s", typeStr, sourceStr, id, sevStr, message);
	log("GL msg type: %s, src: %s, id: %d, severity: %s\nmsg: %s", typeStr, sourceStr, id, sevStr, message);
}

GLenum checkFramebufferStatus(video::Id fbo) {
	GLenum status;
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glCheckNamedFramebufferStatus != nullptr);
		status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
	} else {
		core_assert(glCheckFramebufferStatus != nullptr);
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		return status;
	}
	switch (status) {
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		Log::error("FB error, incomplete attachment");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		Log::error("FB error, incomplete missing attachment");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		Log::error("FB error, incomplete draw buffer");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		Log::error("FB error, incomplete read buffer");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		Log::error("FB error, framebuffer unsupported");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		Log::error("FB error, incomplete multisample");
		break;
	default:
		Log::error("FB error, status: %i", (int)status);
		break;
	}
	return status;
}

void setupLimitsAndSpecs() {
	core_assert(glGetIntegerv != nullptr);
	glGetIntegerv(GL_MAX_SAMPLES, &renderState().limits[core::enumVal(Limit::MaxSamples)]);
	checkError();
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &renderState().limits[core::enumVal(Limit::MaxTextureSize)]);
	checkError();
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &renderState().limits[core::enumVal(Limit::MaxCubeMapTextureSize)]);
	checkError();
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &renderState().limits[core::enumVal(Limit::MaxViewPortWidth)]);
	checkError();
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &renderState().limits[core::enumVal(Limit::MaxDrawBuffers)]);
	checkError();
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &renderState().limits[core::enumVal(Limit::MaxVertexAttribs)]);
	checkError();
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &renderState().limits[core::enumVal(Limit::MaxCombinedTextureImageUnits)]);
	checkError();
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &renderState().limits[core::enumVal(Limit::MaxVertexTextureImageUnits)]);
	checkError();
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &renderState().limits[core::enumVal(Limit::MaxElementIndices)]);
	checkError();
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &renderState().limits[core::enumVal(Limit::MaxElementVertices)]);
	checkError();
	glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &renderState().limits[core::enumVal(Limit::MaxFragmentInputComponents)]);
	checkError();
	if (hasFeature(Feature::ComputeShaders)) {
		core_assert(glGetIntegeri_v != nullptr);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountX)]);
		checkError();
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountY)]);
		checkError();
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupCountZ)]);
		checkError();
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeX)]);
		checkError();
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeY)]);
		checkError();
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupSizeZ)]);
		checkError();
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &renderState().limits[core::enumVal(Limit::MaxComputeWorkGroupInvocations)]);
		checkError();
	}
	if (FLEXT_KHR_debug) {
		glGetIntegerv(GL_MAX_LABEL_LENGTH, &renderState().limits[core::enumVal(Limit::MaxLabelLength)]);
		checkError();
	}
#ifdef GL_MAX_VERTEX_UNIFORM_VECTORS
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &renderState().limits[core::enumVal(Limit::MaxVertexUniformComponents)]);
	checkError();
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &renderState().limits[core::enumVal(Limit::MaxFragmentUniformComponents)]);
	checkError();
#else
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &renderState().limits[core::enumVal(Limit::MaxVertexUniformComponents)]);
	checkError();
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &renderState().limits[core::enumVal(Limit::MaxFragmentUniformComponents)]);
	checkError();
#endif
	if (FLEXT_ARB_texture_filter_anisotropic) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &renderState().flimits[core::enumVal(Limit::MaxAnisotropy)]);
		checkError();
	}
	glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &renderState().flimits[core::enumVal(Limit::MaxLodBias)]);
	checkError();

	GLint uniformBufferAlignment;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignment);
	renderState().specs[core::enumVal(Spec::UniformBufferAlignment)] = uniformBufferAlignment;
	checkError();
	GLint maxUniformBufferSize;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
	renderState().limits[core::enumVal(Limit::MaxUniformBufferSize)] = maxUniformBufferSize;
	checkError();
	GLint maxUniformBufferBindings;
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
	renderState().limits[core::enumVal(Limit::MaxUniformBufferBindings)] = maxUniformBufferBindings;
	checkError();

	if (hasFeature(Feature::ShaderStorageBufferObject)) {
		GLint shaderStorageBufferOffsetAlignment;
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &shaderStorageBufferOffsetAlignment);
		renderState().specs[core::enumVal(Spec::ShaderStorageBufferOffsetAlignment)] = shaderStorageBufferOffsetAlignment;
		checkError();
		GLint maxShaderStorageBlockSize;
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxShaderStorageBlockSize);
		renderState().limits[core::enumVal(Limit::MaxShaderStorageBufferSize)] = maxShaderStorageBlockSize;
		checkError();
	}

	Log::debug("GL_MAX_ELEMENTS_VERTICES: %i", renderState().limits[core::enumVal(Limit::MaxElementVertices)]);
	Log::debug("GL_MAX_ELEMENTS_INDICES: %i", renderState().limits[core::enumVal(Limit::MaxElementIndices)]);
	Log::debug("GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT: %i", (int)renderState().specs[core::enumVal(Spec::UniformBufferAlignment)]);
	Log::debug("GL_MAX_UNIFORM_BLOCK_SIZE: %i", (int)renderState().limits[core::enumVal(Limit::MaxUniformBufferSize)]);
	Log::debug("GL_MAX_UNIFORM_BUFFER_BINDINGS: %i", (int)renderState().limits[core::enumVal(Limit::MaxUniformBufferBindings)]);
}

void setupFeatures(GLVersion version) {
	const core::List<const char *> extensionArray[] = {
		{"GL_ARB_texture_compression_s3tc", "GL_ARB_compressed_texture_s3tc", "GL_ARB_texture_compression_dxt1"},
		{"GL_ARB_texture_compression_pvrtc", "GL_ARB_compressed_texture_pvrtc"},
		{},
		{"GL_ARB_compressed_ATC_texture", "GL_ARB_compressed_texture_atc"},
		{"GL_ARB_texture_float"},
		{"GL_ARB_texture_half_float"},
		{"GL_ARB_instanced_arrays"},
		{"GL_ARB_debug_output"},
		// the primary difference between ARB and EXT is that ARB requires the use of
		// glCreateResource rather than working from glGenResource object handles.
		// https://www.opengl.org/registry/specs/ARB/direct_state_access.txt
		{"GL_ARB_direct_state_access"},
		{"GL_ARB_buffer_storage"},
		{"GL_ARB_multi_draw_indirect"},
		{"GL_ARB_compute_shader"},
		{"GL_ARB_transform_feedback2"},
		{"GL_ARB_shader_storage_buffer_object"}
	};
	static_assert(core::enumVal(Feature::Max) == (int)lengthof(extensionArray), "Array sizes don't match for Feature enum");

	int numExts;
	core_assert(glGetIntegerv != nullptr);
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExts);
	Log::debug("OpenGL extensions:");
	for (int i = 0; i < numExts; ++i) {
		const char *extensionStr = (const char *) glGetStringi(GL_EXTENSIONS, i);
		Log::debug("ext: %s", extensionStr);
	}

	for (size_t i = 0; i < lengthof(extensionArray); ++i) {
		const core::List<const char *>& extStrVector = extensionArray[i];
		for (const char *extStr : extStrVector) {
			renderState().features[i] = SDL_GL_ExtensionSupported(extStr);
			if (renderState().features[i]) {
				Log::debug("Detected feature: %s", extStr);
				break;
			}
		}
	}

	int mask = 0;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask)) {
#else
	if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask) != -1) {
#endif
		if ((mask & SDL_GL_CONTEXT_PROFILE_CORE) != 0) {
			renderState().features[core::enumVal(Feature::TextureCompressionDXT)] = true;
			renderState().features[core::enumVal(Feature::InstancedArrays)] = true;
			renderState().features[core::enumVal(Feature::TextureFloat)] = true;
		}
	}

	// Buffer storage is a core feature in OpenGL 4.4+
	if (version.majorVersion > 4 || (version.majorVersion == 4 && version.minorVersion >= 4)) {
		renderState().features[core::enumVal(Feature::BufferStorage)] = true;
	}

#ifdef GL_CLIP_ORIGIN
	GLenum clipOrigin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&clipOrigin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
	if (clipOrigin == GL_UPPER_LEFT) {
		s.clipOriginLowerLeft = false;
	}
#endif

#ifdef USE_OPENGLES
	renderState().features[core::enumVal(Feature::TextureFloat)] = true;
	renderState().features[core::enumVal(Feature::TextureHalfFloat)] = true;
	renderState().features[core::enumVal(Feature::InstancedArrays)] = true;
	renderState().features[core::enumVal(Feature::TextureCompressionETC2)] = true;
#else
	renderState().features[core::enumVal(Feature::TextureHalfFloat)] = renderState().features[core::enumVal(Feature::TextureFloat)];
#endif
}

int fillUniforms(Id program, ShaderUniforms& uniformMap, const core::String& shaderName, bool block) {
	GLenum activeEnum;
	GLenum activeMaxLengthEnum;
	if (block) {
		activeEnum = GL_ACTIVE_UNIFORM_BLOCKS;
		activeMaxLengthEnum = GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH;
	} else {
		activeEnum = GL_ACTIVE_UNIFORMS;
		activeMaxLengthEnum = GL_ACTIVE_UNIFORM_MAX_LENGTH;
	}
	GLint numUniforms = 0;
	glGetProgramiv(program, activeEnum, &numUniforms);
	GLint uniformNameSize = 0;
	glGetProgramiv(program, activeMaxLengthEnum, &uniformNameSize);
	char name[4096];
	if (uniformNameSize + 1 >= (int)sizeof(name)) {
		return 0;
	}

	const char *shaderNameC = shaderName.c_str();
	for (int i = 0; i < numUniforms; i++) {
		Uniform uniform;
		if (block) {
			core_assert(glGetActiveUniformBlockName != nullptr);
			glGetActiveUniformBlockName(program, i, uniformNameSize, nullptr, name);
			core_assert(glGetUniformBlockIndex != nullptr);
			uint32_t location = glGetUniformBlockIndex(program, name);
			if (location == GL_INVALID_INDEX) {
				Log::debug("Could not get uniform block location for %s is %i (shader %s)", name, location, shaderNameC);
				continue;
			}
			uniform.location = location;
			Log::debug("Got uniform location for %s is %i (shader %s)", name, location, shaderNameC);
		} else {
			GLint size = 0;
			GLenum type = 0;
			core_assert(glGetActiveUniform != nullptr);
			glGetActiveUniform(program, i, uniformNameSize, nullptr, &size, &type, name);
			core_assert(glGetUniformLocation != nullptr);
			int32_t location = glGetUniformLocation(program, name);
			if (location < 0) {
				Log::debug("Could not get uniform location for %s is %i (shader %s)", name, location, shaderNameC);
				continue;
			}
			uniform.location = location;
			Log::debug("Got uniform location for %s is %i (shader %s)", name, location, shaderNameC);
		}
		char* array = SDL_strchr(name, '[');
		if (array != nullptr) {
			*array = '\0';
		}
		uniform.block = block;
		if (block) {
			core_assert(glGetUniformBlockIndex != nullptr);
			uniform.blockIndex = glGetUniformBlockIndex(program, name);
			core_assert(glGetActiveUniformBlockiv != nullptr);
			glGetActiveUniformBlockiv(program, uniform.location, GL_UNIFORM_BLOCK_DATA_SIZE, &uniform.size);
			uniform.blockBinding = i;
		}
		uniformMap.put(core::String(name), uniform);
	}
	return numUniforms;
}

}

static inline _priv::GLState &glState() {
	static _priv::GLState s;
	return s;
}

RendererState &rendererState() {
	return (RendererState &)glState();
}

static void validate(Id handle) {
#ifdef DEBUG
	if (!rendererState().needValidation) {
		return;
	}
	rendererState().needValidation = false;
	const GLuint lid = (GLuint)handle;
	core_assert(glValidateProgram != nullptr);
	glValidateProgram(lid);
	video::checkError();
	GLint success = 0;
	core_assert(glGetProgramiv != nullptr);
	glGetProgramiv(lid, GL_VALIDATE_STATUS, &success);
	video::checkError();
	GLint logLength = 0;
	glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &logLength);
	video::checkError();
	if (logLength > 1) {
		core::String message;
		message.reserve(logLength);
		core_assert(glGetProgramInfoLog != nullptr);
		glGetProgramInfoLog(lid, logLength, nullptr, &message[0]);
		video::checkError();
		if (success == GL_FALSE) {
			Log::error("Validation output: %s", message.c_str());
		} else {
			Log::trace("Validation output: %s", message.c_str());
		}
	}
#endif
}

bool checkError(bool triggerAssert) {
#ifdef DEBUG
	if (glGetError == nullptr) {
		return false;
	}
	bool hasError = false;
	/* check gl errors (can return multiple errors) */
	for (;;) {
		const GLenum glError = glGetError();
		if (glError == GL_NO_ERROR) {
			break;
		}
		const char *error;
		switch (glError) {
		case GL_INVALID_ENUM:
			error = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			error = "GL_INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			error = "GL_OUT_OF_MEMORY";
			break;
		default:
			error = "UNKNOWN";
			break;
		}

		if (triggerAssert) {
			core_assert_msg(glError == GL_NO_ERROR, "GL err: %s => %i", error, glError);
		} else {
			Log::error("GL error: %s (%i)", error, glError);
		}
		/* record that an error was found */
		hasError = true;
	}
	return hasError;
#else
	return false;
#endif
}

// TODO: RENDERER: use FrameBufferConfig
void readBuffer(GBufferTextureType textureType) {
	video_trace_scoped(ReadBuffer);
	core_assert(glReadBuffer != nullptr);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	checkError();
}

float lineWidth(float width) {
	RendererState &rs = rendererState();
	// line width > 1.0 is deprecated in core profile context
	if (glState().glVersion.isAtLeast(3, 2)) {
		return rs.lineWidth;
	}
	video_trace_scoped(LineWidth);
	if (glm::abs(rs.pendingLineWidth - width) < glm::epsilon<float>()) {
		return rs.pendingLineWidth;
	}
	const float oldWidth = rs.pendingLineWidth;
	rs.pendingLineWidth = width;
	return oldWidth;
}

static bool activateTextureUnit(TextureUnit unit) {
	if (rendererState().textureUnit == unit) {
		return false;
	}
	core_assert(TextureUnit::Max != unit);
	const GLenum glUnit = _priv::TextureUnits[core::enumVal(unit)];
	core_assert(glActiveTexture != nullptr);
	glActiveTexture(glUnit);
	checkError();
	rendererState().textureUnit = unit;
	return true;
}

static bool bindTextureForce(TextureUnit unit, TextureType type, Id handle) {
	if (useFeature(Feature::DirectStateAccess)) {
		if (rendererState().textureHandle[core::enumVal(unit)] != handle) {
			rendererState().textureHandle[core::enumVal(unit)] = handle;
			core_assert(glBindTextureUnit != nullptr);
			glBindTextureUnit(core::enumVal(unit), handle);
			checkError();
			return true;
		}
	} else {
		const bool changeUnit = activateTextureUnit(unit);
		if (changeUnit || rendererState().textureHandle[core::enumVal(unit)] != handle) {
			rendererState().textureHandle[core::enumVal(unit)] = handle;
			core_assert(glBindTexture != nullptr);
			glBindTexture(_priv::TextureTypes[core::enumVal(type)], handle);
			checkError();
			return true;
		}
	}
	return false;
}

static void syncState() {
	RendererState &rs = rendererState();

	if (rs.programHandle != rs.pendingProgramHandle) {
		rs.programHandle = rs.pendingProgramHandle;
		core_assert(glUseProgram != nullptr);
		glUseProgram(rs.programHandle);
		checkError();
		rs.needValidation = true;
	}

	// Apply pending uniforms after program is bound
	if (!rs.pendingUniformi.empty()) {
		for (const auto& entry : rs.pendingUniformi) {
			core_assert(glUniform1i != nullptr);
			glUniform1i(entry->first, entry->second);
		}
		checkError();
		rs.pendingUniformi.clear();
	}

	for (int i = 0; i < core::enumVal(TextureUnit::Max); ++i) {
		if (rs.textureHandle[i] != rs.pendingTextureHandle[i]) {
			bindTextureForce((TextureUnit)i, rs.pendingTextureType[i], rs.pendingTextureHandle[i]);
		}
	}

	if (rs.clearColor != rs.pendingClearColor) {
		rs.clearColor = rs.pendingClearColor;
		core_assert(glClearColor != nullptr);
		glClearColor(rs.clearColor.r, rs.clearColor.g, rs.clearColor.b, rs.clearColor.a);
		checkError();
	}

	if (rs.viewportX != rs.pendingViewportX || rs.viewportY != rs.pendingViewportY ||
		rs.viewportW != rs.pendingViewportW || rs.viewportH != rs.pendingViewportH) {
		rs.viewportX = rs.pendingViewportX;
		rs.viewportY = rs.pendingViewportY;
		rs.viewportW = rs.pendingViewportW;
		rs.viewportH = rs.pendingViewportH;
		core_assert(glViewport != nullptr);
		glViewport((GLint)rs.viewportX, (GLint)rs.viewportY, (GLsizei)rs.viewportW, (GLsizei)rs.viewportH);
		checkError();
	}

	if (rs.colorMask[0] != rs.pendingColorMask[0] || rs.colorMask[1] != rs.pendingColorMask[1] ||
		rs.colorMask[2] != rs.pendingColorMask[2] || rs.colorMask[3] != rs.pendingColorMask[3]) {
		core_assert(glColorMask != nullptr);
		glColorMask((GLboolean)rs.pendingColorMask[0], (GLboolean)rs.pendingColorMask[1],
					(GLboolean)rs.pendingColorMask[2], (GLboolean)rs.pendingColorMask[3]);
		rs.colorMask[0] = rs.pendingColorMask[0];
		rs.colorMask[1] = rs.pendingColorMask[1];
		rs.colorMask[2] = rs.pendingColorMask[2];
		rs.colorMask[3] = rs.pendingColorMask[3];
		checkError();
	}

	if (rs.scissorX != rs.pendingScissorX || rs.scissorY != rs.pendingScissorY || rs.scissorW != rs.pendingScissorW ||
		rs.scissorH != rs.pendingScissorH) {
		rs.scissorX = rs.pendingScissorX;
		rs.scissorY = rs.pendingScissorY;
		rs.scissorW = rs.pendingScissorW;
		rs.scissorH = rs.pendingScissorH;

		GLint bottom;
		if (rs.clipOriginLowerLeft) {
			bottom = rs.viewportH - (rs.scissorY + rs.scissorH);
		} else {
			bottom = rs.scissorY;
		}
		bottom = (GLint)glm::round(bottom * rs.scaleFactor);
		const GLint left = (GLint)glm::round(rs.scissorX * rs.scaleFactor);
		const GLsizei width = (GLsizei)glm::round(rs.scissorW * rs.scaleFactor);
		const GLsizei height = (GLsizei)glm::round(rs.scissorH * rs.scaleFactor);
		core_assert(glScissor != nullptr);
		glScissor(left, bottom, width, height);
		checkError();
	}

	if (rs.states != rs.pendingStates) {
		for (int i = 0; i < core::enumVal(State::Max); ++i) {
			if (rs.states[i] != rs.pendingStates[i]) {
				const State state = (State)i;
				const bool enable = rs.pendingStates[i];
				if (state == State::DepthMask) {
					core_assert(glDepthMask != nullptr);
					glDepthMask(enable ? GL_TRUE : GL_FALSE);
				} else {
					if (enable) {
						core_assert(glEnable != nullptr);
						glEnable(_priv::States[i]);
					} else {
						core_assert(glDisable != nullptr);
						glDisable(_priv::States[i]);
					}
				}
			}
		}
		rs.states = rs.pendingStates;
		checkError();
	}

	if (rs.blendEquation != rs.pendingBlendEquation) {
		rs.blendEquation = rs.pendingBlendEquation;
		const GLenum convertedFunc = _priv::BlendEquations[core::enumVal(rs.blendEquation)];
		core_assert(glBlendEquation != nullptr);
		glBlendEquation(convertedFunc);
		checkError();
	}

	if (rs.blendSrcRGB != rs.pendingBlendSrcRGB || rs.blendDestRGB != rs.pendingBlendDestRGB ||
		rs.blendSrcAlpha != rs.pendingBlendSrcAlpha || rs.blendDestAlpha != rs.pendingBlendDestAlpha) {
		rs.blendSrcRGB = rs.pendingBlendSrcRGB;
		rs.blendDestRGB = rs.pendingBlendDestRGB;
		rs.blendSrcAlpha = rs.pendingBlendSrcAlpha;
		rs.blendDestAlpha = rs.pendingBlendDestAlpha;

		if (rs.blendSrcRGB == rs.blendSrcAlpha && rs.blendDestRGB == rs.blendDestAlpha) {
			const GLenum glSrc = _priv::BlendModes[core::enumVal(rs.blendSrcRGB)];
			const GLenum glDest = _priv::BlendModes[core::enumVal(rs.blendDestRGB)];
			core_assert(glBlendFunc != nullptr);
			glBlendFunc(glSrc, glDest);
		} else {
			const GLenum glSrcRGB = _priv::BlendModes[core::enumVal(rs.blendSrcRGB)];
			const GLenum glDestRGB = _priv::BlendModes[core::enumVal(rs.blendDestRGB)];
			const GLenum glSrcAlpha = _priv::BlendModes[core::enumVal(rs.blendSrcAlpha)];
			const GLenum glDestAlpha = _priv::BlendModes[core::enumVal(rs.blendDestAlpha)];
			core_assert(glBlendFuncSeparate != nullptr);
			glBlendFuncSeparate(glSrcRGB, glDestRGB, glSrcAlpha, glDestAlpha);
		}
		checkError();
	}

	if (rs.cullFace != rs.pendingCullFace) {
		rs.cullFace = rs.pendingCullFace;
		const GLenum glFace = _priv::Faces[core::enumVal(rs.cullFace)];
		core_assert(glCullFace != nullptr);
		glCullFace(glFace);
		checkError();
	}

	if (rs.depthFunc != rs.pendingDepthFunc) {
		rs.depthFunc = rs.pendingDepthFunc;
		core_assert(glDepthFunc != nullptr);
		glDepthFunc(_priv::CompareFuncs[core::enumVal(rs.depthFunc)]);
		checkError();
	}

	if (rs.polygonModeFace != rs.pendingPolygonModeFace || rs.polygonMode != rs.pendingPolygonMode) {
		rs.polygonModeFace = rs.pendingPolygonModeFace;
		rs.polygonMode = rs.pendingPolygonMode;
#ifndef USE_OPENGLES
		const GLenum glMode = _priv::PolygonModes[core::enumVal(rs.polygonMode)];
		const GLenum glFace = _priv::Faces[core::enumVal(rs.polygonModeFace)];
		glPolygonMode(glFace, glMode);
		checkError();
#endif
	}

	if (rs.polygonOffset != rs.pendingPolygonOffset) {
		rs.polygonOffset = rs.pendingPolygonOffset;
		core_assert(glPolygonOffset != nullptr);
		glPolygonOffset(rs.polygonOffset.x, rs.polygonOffset.y);
		checkError();
	}

	if (rs.pointSize != rs.pendingPointSize) {
		rs.pointSize = rs.pendingPointSize;
		core_assert(glPointSize != nullptr);
		glPointSize(rs.pointSize);
		checkError();
	}

	if (glm::abs(rs.lineWidth - rs.pendingLineWidth) >= glm::epsilon<float>()) {
		float width = rs.pendingLineWidth;
		if (rs.states[core::enumVal(State::LineSmooth)]) {
			width = glm::clamp(width, rs.smoothedLineWidth.x, rs.smoothedLineWidth.y);
		} else {
			width = glm::clamp(width, rs.aliasedLineWidth.x, rs.aliasedLineWidth.y);
		}
		core_assert(glLineWidth != nullptr);
		glLineWidth((GLfloat)width);
		checkError(false);
		rs.lineWidth = rs.pendingLineWidth;
	}
}

static GLbitfield getBitField(ClearFlag flag) {
	GLbitfield glValue = 0;
	if ((flag & ClearFlag::Color) == ClearFlag::Color) {
		glValue |= GL_COLOR_BUFFER_BIT;
	}
	if ((flag & ClearFlag::Stencil) == ClearFlag::Stencil) {
		glValue |= GL_STENCIL_BUFFER_BIT;
	}
	if ((flag & ClearFlag::Depth) == ClearFlag::Depth) {
		glValue |= GL_DEPTH_BUFFER_BIT;
	}
	return glValue;
}

void clear(ClearFlag flag) {
	video_trace_scoped(Clear);
	const GLbitfield glValue = getBitField(flag);
	if (glValue == 0) {
		return;
	}
	syncState();
	// intel told me so... 5% performance gain if clear is called with disabled scissors.
	const bool enabled = disable(State::Scissor);
	core_assert(glClear != nullptr);
	glClear(glValue);
	if (enabled) {
		enable(State::Scissor);
	}
	checkError();
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	core_assert(TextureUnit::Max != unit);
	core_assert(TextureType::Max != type);
	bool changed = rendererState().pendingTextureHandle[core::enumVal(unit)] != handle;
	rendererState().pendingTextureHandle[core::enumVal(unit)] = handle;
	rendererState().pendingTextureType[core::enumVal(unit)] = type;

	if (unit == TextureUnit::Upload) {
		bindTextureForce(unit, type, handle);
	}
	return changed;
}

bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels) {
	video_trace_scoped(ReadTexture);
	const _priv::Formats &f = _priv::textureFormats[core::enumVal(format)];
	const int pitch = w * f.bits / 8;
	*pixels = (uint8_t *)core_malloc(h * pitch);
	core_assert(glPixelStorei != nullptr);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	if (useFeature(Feature::DirectStateAccess) && glGetTextureImage != nullptr) {
		glGetTextureImage(handle,
						  0,			// mipmap level
						  f.dataFormat, // e.g. GL_RGBA
						  f.dataType,	// e.g. GL_UNSIGNED_BYTE
						  h * pitch,	// size of the destination buffer
						  *pixels		// destination pointer

		);
	} else if (!useFeature(Feature::DirectStateAccess) && glGetTexImage != nullptr) {
		bindTextureForce(unit, type, handle);
		glGetTexImage(_priv::TextureTypes[core::enumVal(type)], 0, f.dataFormat, f.dataType, (void *)*pixels);
	} else {
		/* Fallback for WebGL / OpenGLES where glGetTexImage / glGetTextureImage
		 * are not available: create a temporary FBO, attach the texture and
		 * use glReadPixels to read the pixels. This covers 2D and cube faces;
		 * for other types this may not be supported. */
		GLuint oldFbo = 0;
		core_assert(glGetIntegerv != nullptr);
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&oldFbo);
		GLuint tmpFbo = 0;
		core_assert(glGenFramebuffers != nullptr);
		glGenFramebuffers(1, &tmpFbo);
		core_assert(glBindFramebuffer != nullptr);
		glBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
		checkError();

		/* Attach depending on texture type. Prefer POSITIVE_X face for cubes. */
		if (type == TextureType::TextureCube) {
			core_assert(glFramebufferTexture2D != nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, (GLuint)handle,
								   0);
		} else if (type == TextureType::Texture2D || type == TextureType::Texture2DMultisample) {
			core_assert(glFramebufferTexture2D != nullptr);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)handle, 0);
		} else if (type == TextureType::Texture2DArray || type == TextureType::Texture3D) {
			/* attach layer 0 */
			core_assert(glFramebufferTextureLayer != nullptr);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLuint)handle, 0, 0);
		} else {
			/* fallback to generic framebuffer texture attach if available */
			if (glFramebufferTexture != nullptr) {
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLuint)handle, 0);
			} else {
				/* unsupported texture type for fallback */
				Log::error("readTexture: unsupported texture type for fallback read");
				glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
				glDeleteFramebuffers(1, &tmpFbo);
				core_free(*pixels);
				*pixels = nullptr;
				return false;
			}
		}
		/* set read buffer and read */
		core_assert(glReadBuffer != nullptr);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		core_assert(glReadPixels != nullptr);
		glReadPixels(0, 0, w, h, f.dataFormat, f.dataType, (void *)*pixels);
		/* restore */
		glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
		glDeleteFramebuffers(1, &tmpFbo);
	}
	if (checkError()) {
		core_free(*pixels);
		*pixels = nullptr;
		return false;
	}
	return true;
}

bool bindVertexArray(Id handle) {
	if (rendererState().vertexArrayHandle == handle) {
		return false;
	}
	core_assert(glBindVertexArray != nullptr);
	glBindVertexArray(handle);
	checkError();
	rendererState().vertexArrayHandle = handle;
	return true;
}

void *mapBuffer(Id handle, BufferType type, AccessMode mode) {
	video_trace_scoped(MapBuffer);
	const int modeIndex = core::enumVal(mode);
	const GLenum glMode = _priv::AccessModes[modeIndex];
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glMapNamedBuffer != nullptr);
		void *data = glMapNamedBuffer(handle, glMode);
		checkError();
		return data;
	}
	bindBuffer(type, handle);
	const int typeIndex = core::enumVal(type);
	const GLenum glType = _priv::BufferTypes[typeIndex];
	core_assert(glMapBuffer != nullptr);
	void *data = glMapBuffer(glType, glMode);
	checkError();
	unbindBuffer(type);
	return data;
}

void *mapBufferRange(Id handle, BufferType type, intptr_t offset, size_t length, AccessMode mode, MapBufferFlag flags) {
	video_trace_scoped(MapBufferRange);
	if (length == 0 || handle == InvalidId) {
		return nullptr;
	}
	GLenum access = 0;
	switch (mode) {
	case AccessMode::Read:
		access |= GL_MAP_READ_BIT;
		break;
	case AccessMode::Write:
		access |= GL_MAP_WRITE_BIT;
		break;
	case AccessMode::ReadWrite:
		access |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		break;
	default:
		access |= GL_MAP_WRITE_BIT;
		break;
	}

	if ((flags & MapBufferFlag::InvalidateRange) == MapBufferFlag::InvalidateRange) {
		access |= GL_MAP_INVALIDATE_RANGE_BIT;
	}
	if ((flags & MapBufferFlag::Unsynchronized) == MapBufferFlag::Unsynchronized) {
		access |= GL_MAP_UNSYNCHRONIZED_BIT;
	}
	if ((flags & MapBufferFlag::ExplicitFlush) == MapBufferFlag::ExplicitFlush) {
		access |= GL_MAP_FLUSH_EXPLICIT_BIT;
	}

	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glMapNamedBufferRange != nullptr);
		void *ptr = glMapNamedBufferRange((GLuint)handle, (GLintptr)offset, (GLsizeiptr)length, access);
		checkError();
		return ptr;
	}

	const int typeIndex = core::enumVal(type);
	const GLenum glType = _priv::BufferTypes[typeIndex];
	const Id old = boundBuffer(type);
	const bool changed = bindBuffer(type, handle);
	core_assert(glMapBufferRange != nullptr);
	void *ptr = glMapBufferRange(glType, (GLintptr)offset, (GLsizeiptr)length, access);
	checkError();
	if (changed) {
		if (old == InvalidId) {
			unbindBuffer(type);
		} else {
			bindBuffer(type, old);
		}
	}
	return ptr;
}

void unmapBuffer(Id handle, BufferType type) {
	video_trace_scoped(UnmapBuffer);
	if (handle == InvalidId) {
		return;
	}
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glUnmapNamedBuffer != nullptr);
		core_assert(glUnmapNamedBuffer((GLuint)handle) == GL_TRUE);
		checkError();
		return;
	}

	const int typeIndex = core::enumVal(type);
	const GLenum glType = _priv::BufferTypes[typeIndex];
	const Id oldBuffer = boundBuffer(type);
	const bool changed = bindBuffer(type, handle);
	core_assert(glUnmapBuffer != nullptr);
	core_assert_always(glUnmapBuffer(glType) == GL_TRUE);
	checkError();
	if (changed) {
		if (oldBuffer == InvalidId) {
			unbindBuffer(type);
		} else {
			bindBuffer(type, oldBuffer);
		}
	}
}

bool bindBuffer(BufferType type, Id handle) {
	video_trace_scoped(BindBuffer);
	const int typeIndex = core::enumVal(type);
	if (rendererState().bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	rendererState().bufferHandle[typeIndex] = handle;
	core_assert(handle != InvalidId);
	core_assert(glBindBuffer != nullptr);
	glBindBuffer(glType, handle);
	checkError();
	return true;
}

bool unbindBuffer(BufferType type) {
	const int typeIndex = core::enumVal(type);
	if (rendererState().bufferHandle[typeIndex] == InvalidId) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	rendererState().bufferHandle[typeIndex] = InvalidId;
	core_assert(glBindBuffer != nullptr);
	glBindBuffer(glType, InvalidId);
	checkError();
	return true;
}

bool bindBufferBase(BufferType type, Id handle, uint32_t index) {
	video_trace_scoped(BindBufferBase);
	const int typeIndex = core::enumVal(type);
	// Cache key: combine type and index to track per-binding-point state
	const uint64_t key = (static_cast<uint64_t>(typeIndex) << 32) | index;
	Id cachedHandle = InvalidId;
	if (rendererState().bufferBaseBindings.get(key, cachedHandle)) {
		if (cachedHandle == handle) {
			return false; // binding already set, avoid redundant call
		}
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	rendererState().bufferHandle[typeIndex] = handle;
	core_assert(glBindBufferBase != nullptr);
	glBindBufferBase(glType, (GLuint)index, handle);
	checkError();
	rendererState().bufferBaseBindings.put(key, handle);
	return true;
}

void genBuffers(uint8_t amount, Id *ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glCreateBuffers != nullptr);
		glCreateBuffers((GLsizei)amount, (GLuint *)ids);
		checkError();
	} else {
		core_assert(glGenBuffers != nullptr);
		glGenBuffers((GLsizei)amount, (GLuint *)ids);
		checkError();
	}
}

void deleteBuffers(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	for (uint8_t i = 0u; i < amount; ++i) {
		for (int j = 0; j < lengthof(rendererState().bufferHandle); ++j) {
			if (rendererState().bufferHandle[j] == ids[i]) {
				rendererState().bufferHandle[j] = InvalidId;
			}
		}
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	core_assert(glDeleteBuffers != nullptr);
	glDeleteBuffers((GLsizei)amount, (GLuint *)ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void genVertexArrays(uint8_t amount, Id *ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glCreateVertexArrays != nullptr);
		glCreateVertexArrays((GLsizei)amount, (GLuint *)ids);
	} else {
		core_assert(glGenVertexArrays != nullptr);
		glGenVertexArrays((GLsizei)amount, (GLuint *)ids);
	}
	checkError();
}

void deleteShader(Id &id) {
	if (id == InvalidId) {
		return;
	}
	core_assert(glIsShader != nullptr);
	core_assert(glDeleteShader != nullptr);
	core_assert_msg(glIsShader((GLuint)id), "%u is no valid shader object", (unsigned int)id);
	glDeleteShader((GLuint)id);
	Log::debug("delete %u shader object", (unsigned int)id);
	checkError();
	id = InvalidId;
}

Id genShader(ShaderType type) {
	if (glCreateShader == nullptr) {
		return InvalidId;
	}
	const GLenum glType = _priv::ShaderTypes[core::enumVal(type)];
	const Id id = (Id)glCreateShader(glType);
	Log::debug("create %u shader object", (unsigned int)id);
	checkError();
	return id;
}

void deleteProgram(Id &id) {
	if (id == InvalidId) {
		return;
	}
	core_assert(glIsShader != nullptr);
	core_assert(glDeleteProgram != nullptr);
	core_assert_msg(glIsProgram((GLuint)id), "%u is no valid program object", (unsigned int)id);
	glDeleteProgram((GLuint)id);
	Log::debug("delete %u shader program", (unsigned int)id);
	checkError();
	if (rendererState().programHandle == id) {
		rendererState().programHandle = InvalidId;
	}
	if (rendererState().pendingProgramHandle == id) {
		rendererState().pendingProgramHandle = InvalidId;
	}
	// Clear cached uniform buffer bindings for this program to prevent memory leak
	// Keys are (program << 32 | blockIndex), so we need to remove all with matching program
	const uint64_t programMask = static_cast<uint64_t>(id) << 32;
	const uint64_t programBits = 0xFFFFFFFF00000000ULL;
	core::DynamicArray<uint64_t> keysToRemove;
	for (auto it = rendererState().uniformBufferBindings.begin(); it != rendererState().uniformBufferBindings.end(); ++it) {
		if (((*it)->key & programBits) == programMask) {
			keysToRemove.push_back((*it)->key);
		}
	}
	for (uint64_t key : keysToRemove) {
		rendererState().uniformBufferBindings.remove(key);
	}
	id = InvalidId;
}

Id genProgram() {
	checkError();
	core_assert(glCreateProgram != nullptr);
	Id id = (Id)glCreateProgram();
	Log::debug("create %u shader program", (unsigned int)id);
	checkError();
	return id;
}

void deleteVertexArrays(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	for (int i = 0; i < amount; ++i) {
		if (rendererState().vertexArrayHandle == ids[i]) {
			bindVertexArray(InvalidId);
			break;
		}
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	core_assert(glDeleteVertexArrays != nullptr);
	glDeleteVertexArrays((GLsizei)amount, (GLuint *)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void genTextures(const TextureConfig &cfg, uint8_t amount, Id *ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (useFeature(Feature::DirectStateAccess)) {
		const GLenum glTexType = _priv::TextureTypes[core::enumVal(cfg.type())];
		core_assert(glCreateTextures != nullptr);
		glCreateTextures(glTexType, (GLsizei)amount, (GLuint *)ids);
		checkError();
	} else {
		core_assert(glGenTextures != nullptr);
		glGenTextures((GLsizei)amount, (GLuint *)ids);
		checkError();
	}
	for (int i = 0; i < amount; ++i) {
		rendererState().textures.insert(ids[i]);
	}
}

void deleteTextures(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	core_assert(glDeleteTextures != nullptr);
	glDeleteTextures((GLsizei)amount, (GLuint *)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		rendererState().textures.remove(ids[i]);
		for (int j = 0; j < lengthof(rendererState().textureHandle); ++j) {
			if (rendererState().textureHandle[j] == ids[i]) {
				// the texture might still be bound...
				rendererState().textureHandle[j] = InvalidId;
			}
		}
		ids[i] = InvalidId;
	}
}

void setObjectName(Id handle, ObjectNameType type, const core::String &name) {
#if 0
	// TODO: this is throwing a lot of GL_INVALID_VALUE errors
	if (handle == InvalidId || name.empty()) {
		return;
	}
	if (glObjectLabel != nullptr) {
		const GLenum glidentifier = _priv::ObjectNameTypes[(int)type];
		const GLuint glname = (GLuint)handle;
		const GLsizei gllength = (GLsizei)name.size();
		if (gllength >= limit(Limit::MaxLabelLength)) {
			return;
		}
		const GLchar *gllabel = (const GLchar *)name.c_str();
		glObjectLabel(glidentifier, glname, gllength, gllabel);
		checkError();
	}
#endif
}

void genFramebuffers(uint8_t amount, Id *ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glCreateFramebuffers != nullptr);
		glCreateFramebuffers((GLsizei)amount, (GLuint *)ids);
	} else {
		core_assert(glGenFramebuffers != nullptr);
		glGenFramebuffers((GLsizei)amount, (GLuint *)ids);
	}
	checkError();
}

void deleteFramebuffers(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	for (int i = 0; i < amount; ++i) {
		if (ids[i] == currentFramebuffer()) {
			bindFramebuffer(InvalidId);
		}
		ids[i] = InvalidId;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	core_assert(glDeleteFramebuffers != nullptr);
	glDeleteFramebuffers((GLsizei)amount, (const GLuint *)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void genRenderbuffers(uint8_t amount, Id *ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glCreateRenderbuffers != nullptr);
		glCreateRenderbuffers((GLsizei)amount, (GLuint *)ids);
	} else {
		core_assert(glGenRenderbuffers != nullptr);
		glGenRenderbuffers((GLsizei)amount, (GLuint *)ids);
	}
	checkError();
}

void deleteRenderbuffers(uint8_t amount, Id *ids) {
	if (amount == 0) {
		return;
	}
	for (uint8_t i = 0u; i < amount; ++i) {
		if (rendererState().renderBufferHandle == ids[i]) {
			bindRenderbuffer(InvalidId);
		}
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	core_assert(glDeleteRenderbuffers != nullptr);
	glDeleteRenderbuffers((GLsizei)amount, (GLuint *)ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

#define GL_OFFSET_CAST(i) ((void*)(i))

void configureAttribute(const Attribute &a) {
	video_trace_scoped(ConfigureVertexAttribute);
	// The program doesn't need to be bound yet - VAO state is independent
	// But we should have a program pending to ensure the attributes make sense
	core_assert(rendererState().pendingProgramHandle != InvalidId || rendererState().programHandle != InvalidId);
	core_assert(glEnableVertexAttribArray != nullptr);
	glEnableVertexAttribArray(a.location);
	checkError();
	const GLenum glType = _priv::DataTypes[core::enumVal(a.type)];
	if (a.typeIsInt) {
		core_assert(glVertexAttribIPointer != nullptr);
		glVertexAttribIPointer(a.location, a.size, glType, a.stride, GL_OFFSET_CAST(a.offset));
		checkError();
	} else {
		core_assert(glVertexAttribPointer != nullptr);
		glVertexAttribPointer(a.location, a.size, glType, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
		checkError();
	}
	if (a.divisor > 0) {
		core_assert(glVertexAttribDivisor != nullptr);
		glVertexAttribDivisor(a.location, a.divisor);
		checkError();
	}
}

#undef GL_OFFSET_CAST

void flush() {
	video_trace_scoped(Flush);
	core_assert(glFlush != nullptr);
	glFlush();
	checkError();
}

void finish() {
	video_trace_scoped(Finish);
	core_assert(glFinish != nullptr);
	glFinish();
	checkError();
}

void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height) {
	syncState();
	const GLbitfield glValue = getBitField(flag);
	GLenum filter = GL_NEAREST;
	if (flag == ClearFlag::Color) {
		filter = GL_LINEAR;
	}
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glBlitNamedFramebuffer != nullptr);
		glBlitNamedFramebuffer(handle, target, 0, 0, width, height, 0, 0, width, height, glValue, filter);
		checkError();
	} else {
		video::bindFramebuffer(target, FrameBufferMode::Draw);
		video::bindFramebuffer(handle, FrameBufferMode::Read);
		core_assert(glBlitFramebuffer != nullptr);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, glValue, filter);
		checkError();
		video::bindFramebuffer(handle, FrameBufferMode::Default);
		video::bindFramebuffer(target, FrameBufferMode::Default);
	}
}

Id bindFramebuffer(Id handle, FrameBufferMode mode) {
	const Id old = rendererState().framebufferHandle;
#if SANITY_CHECKS_GL
	GLint _oldFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	core_assert_always(_oldFramebuffer == (GLint)old);
#endif
	if (old == handle && mode == rendererState().framebufferMode) {
		return handle;
	}
	rendererState().framebufferHandle = handle;
	rendererState().framebufferMode = mode;
	const int typeIndex = core::enumVal(mode);
	const GLenum glType = _priv::FrameBufferModes[typeIndex];
	core_assert(glBindFramebuffer != nullptr);
	glBindFramebuffer(glType, handle);
	checkError();
	return old;
}

bool setupRenderBuffer(Id rbo, TextureFormat format, int w, int h, int samples) {
	video_trace_scoped(SetupRenderBuffer);

	if (useFeature(Feature::DirectStateAccess)) {
		if (samples > 0) {
			core_assert(glNamedRenderbufferStorageMultisample != nullptr);
			glNamedRenderbufferStorageMultisample(rendererState().renderBufferHandle, (GLsizei)samples,
												  _priv::TextureFormats[core::enumVal(format)], w, h);
			checkError();
		} else {
			core_assert(glNamedRenderbufferStorage != nullptr);
			glNamedRenderbufferStorage(rendererState().renderBufferHandle, _priv::TextureFormats[core::enumVal(format)],
									   w, h);
			checkError();
		}
	} else {
		if (samples > 0) {
			core_assert(glRenderbufferStorageMultisample != nullptr);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)samples,
											 _priv::TextureFormats[core::enumVal(format)], w, h);
			checkError();
		} else {
			core_assert(glRenderbufferStorage != nullptr);
			glRenderbufferStorage(GL_RENDERBUFFER, _priv::TextureFormats[core::enumVal(format)], w, h);
			checkError();
		}
	}
	return true;
}

Id bindRenderbuffer(Id handle) {
	if (rendererState().renderBufferHandle == handle) {
		return handle;
	}
	const Id prev = rendererState().renderBufferHandle;
	const GLuint lid = (GLuint)handle;
	rendererState().renderBufferHandle = handle;
	if (!useFeature(Feature::DirectStateAccess)) {
		core_assert(glBindRenderbuffer != nullptr);
		glBindRenderbuffer(GL_RENDERBUFFER, lid);
	}
	checkError();
	return prev;
}

void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size) {
	video_trace_scoped(BufferData);
	if (size <= 0) {
		return;
	}
	core_assert_msg(type != BufferType::UniformBuffer || limiti(Limit::MaxUniformBufferSize) <= 0 ||
						(int)size <= limiti(Limit::MaxUniformBufferSize),
					"Given size %i exceeds the max allowed of %i", (int)size, limiti(Limit::MaxUniformBufferSize));
	const GLuint lid = (GLuint)handle;
	const GLenum usage = _priv::BufferModes[core::enumVal(mode)];
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glNamedBufferData != nullptr);
		glNamedBufferData(lid, (GLsizeiptr)size, data, usage);
		checkError();
	} else {
		const GLenum glType = _priv::BufferTypes[core::enumVal(type)];
		const Id oldBuffer = boundBuffer(type);
		const bool changed = bindBuffer(type, handle);
		core_assert(glBufferData != nullptr);
		glBufferData(glType, (GLsizeiptr)size, data, usage);
		checkError();
		if (changed) {
			if (oldBuffer == InvalidId) {
				unbindBuffer(type);
			} else {
				bindBuffer(type, oldBuffer);
			}
		}
	}
	if (rendererState().vendor[core::enumVal(Vendor::Nouveau)]) {
		// nouveau needs this if doing the buffer update short before the draw call
		core_assert(glFlush != nullptr);
		glFlush(); // TODO: RENDERER: use glFenceSync here glClientWaitSync
	}
	checkError();
}

void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size) {
	video_trace_scoped(BufferSubData);
	if (size == 0) {
		return;
	}
	const int typeIndex = core::enumVal(type);
	if (useFeature(Feature::DirectStateAccess)) {
		const GLuint lid = (GLuint)handle;
		core_assert(glNamedBufferSubData != nullptr);
		glNamedBufferSubData(lid, (GLintptr)offset, (GLsizeiptr)size, data);
		checkError();
	} else {
		const GLenum glType = _priv::BufferTypes[typeIndex];
		const Id oldBuffer = boundBuffer(type);
		const bool changed = bindBuffer(type, handle);
		core_assert(glBufferSubData != nullptr);
		glBufferSubData(glType, (GLintptr)offset, (GLsizeiptr)size, data);
		checkError();
		if (changed) {
			if (oldBuffer == InvalidId) {
				unbindBuffer(type);
			} else {
				bindBuffer(type, oldBuffer);
			}
		}
	}
}

// the fbo is flipped in memory, we have to deal with it here
const glm::vec4 &framebufferUV() {
	static const glm::vec4 uv(0.0f, 1.0f, 1.0, 0.0f);
	return uv;
}

bool setupFramebuffer(Id fbo, const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)],
					  const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]) {
	video_trace_scoped(SetupFramebuffer);
	core::Buffer<GLenum> attachments;
	attachments.reserve(core::enumVal(FrameBufferAttachment::Max));

	if (useFeature(Feature::DirectStateAccess)) {
		for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
			if (!bufferAttachments[i]) {
				continue;
			}
			const GLenum glAttachmentType = _priv::FrameBufferAttachments[i];
			core_assert(glNamedFramebufferRenderbuffer != nullptr);
			glNamedFramebufferRenderbuffer(fbo, glAttachmentType, GL_RENDERBUFFER, bufferAttachments[i]->handle());
			checkError();
			if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
				attachments.push_back(glAttachmentType);
			}
		}

		for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
			if (!colorTextures[i]) {
				continue;
			}
			const TextureType textureTarget = colorTextures[i]->type();
			const GLenum glAttachmentType = _priv::FrameBufferAttachments[i];
			const video::Id textureId = colorTextures[i]->handle();
			if (textureTarget == TextureType::TextureCube) {
				core_assert(glNamedFramebufferTextureLayer != nullptr);
				// TODO: RENDERER: Pass correct face or loop over 6 faces
				glNamedFramebufferTextureLayer(fbo, glAttachmentType, textureId, GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
				checkError();
			} else if (textureTarget == TextureType::Texture2D || textureTarget == TextureType::Texture2DMultisample) {
				core_assert(glNamedFramebufferTexture != nullptr);
				glNamedFramebufferTexture(fbo, glAttachmentType, textureId, 0);
				checkError();
			} else {
				core_assert(textureTarget == TextureType::Texture3D || textureTarget == TextureType::Texture2DArray ||
							textureTarget == TextureType::Texture2DMultisampleArray);
				core_assert(glNamedFramebufferTextureLayer != nullptr);
				glNamedFramebufferTextureLayer(fbo, glAttachmentType, textureId, 0, 0);
			}
			if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
				attachments.push_back(glAttachmentType);
			}
		}
		if (attachments.empty()) {
			GLenum buffers[] = {GL_NONE};
			core_assert(glNamedFramebufferDrawBuffers != nullptr);
			glNamedFramebufferDrawBuffers(fbo, lengthof(buffers), buffers);
			checkError();
		} else {
			if (!checkLimit(attachments.size(), Limit::MaxDrawBuffers)) {
				Log::warn("Max draw buffers exceeded");
				return false;
			}
			attachments.sort([](GLenum lhs, GLenum rhs) { return lhs > rhs; });
			core_assert(glNamedFramebufferDrawBuffers != nullptr);
			glNamedFramebufferDrawBuffers(fbo, (GLsizei)attachments.size(), attachments.data());
			checkError();
		}
	} else {
		for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
			if (!bufferAttachments[i]) {
				continue;
			}
			const GLenum glAttachmentType = _priv::FrameBufferAttachments[i];
			core_assert(glFramebufferRenderbuffer != nullptr);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttachmentType, GL_RENDERBUFFER,
									  bufferAttachments[i]->handle());
			checkError();
			if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
				attachments.push_back(glAttachmentType);
			}
		}

		for (int i = 0; i < core::enumVal(FrameBufferAttachment::Max); ++i) {
			if (!colorTextures[i]) {
				continue;
			}
			const TextureType textureTarget = colorTextures[i]->type();
			const GLenum glAttachmentType = _priv::FrameBufferAttachments[i];
			const video::Id textureId = colorTextures[i]->handle();
			if (textureTarget == TextureType::TextureCube) {
				core_assert(glFramebufferTexture2D != nullptr);
				// TODO: RENDERER: Pass correct face or loop over 6 faces
				glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X, textureId, 0);
				checkError();
			} else if (textureTarget == TextureType::Texture2D) {
				core_assert(glFramebufferTexture2D != nullptr);
				glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType, GL_TEXTURE_2D, textureId, 0);
				checkError();
			} else if (textureTarget == TextureType::Texture2DMultisample) {
				core_assert(glFramebufferTexture2D != nullptr);
				glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType, GL_TEXTURE_2D_MULTISAMPLE, textureId, 0);
				checkError();
			} else {
				core_assert(textureTarget == TextureType::Texture3D || textureTarget == TextureType::Texture2DArray ||
							textureTarget == TextureType::Texture2DMultisampleArray);
				core_assert(glFramebufferTextureLayer != nullptr);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttachmentType, textureId, 0, 0);
			}
			if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
				attachments.push_back(glAttachmentType);
			}
		}
		if (attachments.empty()) {
			GLenum buffers[] = {GL_NONE};
			core_assert(glDrawBuffers != nullptr);
			glDrawBuffers(lengthof(buffers), buffers);
			checkError();
		} else {
			if (!checkLimit(attachments.size(), Limit::MaxDrawBuffers)) {
				Log::warn("Max draw buffers exceeded");
				return false;
			}
			attachments.sort([](GLenum lhs, GLenum rhs) { return lhs > rhs; });
			core_assert(glDrawBuffers != nullptr);
			glDrawBuffers((GLsizei)attachments.size(), attachments.data());
			checkError();
		}
	}
	const GLenum status = _priv::checkFramebufferStatus(fbo);
	return status == GL_FRAMEBUFFER_COMPLETE;
}

bool bindFrameBufferAttachment(Id fbo, Id texture, FrameBufferAttachment attachment, int layerIndex, bool shouldClear) {
	video_trace_scoped(BindFrameBufferAttachment);
	const GLenum glAttachment = _priv::FrameBufferAttachments[core::enumVal(attachment)];

	const bool textureLayer = attachment == FrameBufferAttachment::Depth ||
							  attachment == FrameBufferAttachment::Stencil ||
							  attachment == FrameBufferAttachment::DepthStencil;
	if (useFeature(Feature::DirectStateAccess)) {
		if (textureLayer) {
			core_assert(glNamedFramebufferTextureLayer != nullptr);
			glNamedFramebufferTextureLayer(fbo, glAttachment, (GLuint)texture, 0, layerIndex);
			checkError();
		} else {
			core_assert(glNamedFramebufferTexture != nullptr);
			glNamedFramebufferTexture(fbo, glAttachment, (GLuint)texture, 0);
			checkError();
		}
	} else {
		if (textureLayer) {
			core_assert(glFramebufferTextureLayer != nullptr);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttachment, (GLuint)texture, 0, layerIndex);
			checkError();
		} else {
			core_assert(glFramebufferTexture != nullptr);
			glFramebufferTexture(GL_FRAMEBUFFER, glAttachment, (GLuint)texture, 0);
			checkError();
		}
	}
	if (shouldClear) {
		if (attachment == FrameBufferAttachment::Depth) {
			clear(ClearFlag::Depth);
		} else if (attachment == FrameBufferAttachment::Stencil) {
			clear(ClearFlag::Stencil);
		} else if (attachment == FrameBufferAttachment::DepthStencil) {
			clear(ClearFlag::Depth | ClearFlag::Stencil);
		} else {
			clear(ClearFlag::Color);
		}
	}
	const GLenum status = _priv::checkFramebufferStatus(fbo);
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void setupTexture(Id texture, const TextureConfig &config) {
	video_trace_scoped(SetupTexture);
	const GLenum glType = _priv::TextureTypes[core::enumVal(config.type())];
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glTextureParameteri != nullptr);
		core_assert(glTextureParameterfv != nullptr);
		if (config.type() != TextureType::Texture2DMultisample && config.filterMag() != TextureFilter::Max) {
			const GLenum glFilterMag = _priv::TextureFilters[core::enumVal(config.filterMag())];
			glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, glFilterMag);
			checkError();
		}
		if (config.type() != TextureType::Texture2DMultisample && config.filterMin() != TextureFilter::Max) {
			const GLenum glFilterMin = _priv::TextureFilters[core::enumVal(config.filterMin())];
			glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, glFilterMin);
			checkError();
		}
		if (config.type() == TextureType::Texture3D && config.wrapR() != TextureWrap::Max) {
			const GLenum glWrapR = _priv::TextureWraps[core::enumVal(config.wrapR())];
			glTextureParameteri(texture, GL_TEXTURE_WRAP_R, glWrapR);
			checkError();
		}
		if ((config.type() == TextureType::Texture2D || config.type() == TextureType::Texture3D) &&
			config.wrapS() != TextureWrap::Max) {
			const GLenum glWrapS = _priv::TextureWraps[core::enumVal(config.wrapS())];
			glTextureParameteri(texture, GL_TEXTURE_WRAP_S, glWrapS);
			checkError();
		}
		if (config.compareMode() != TextureCompareMode::Max) {
			const GLenum glMode = _priv::TextureCompareModes[core::enumVal(config.compareMode())];
			glTextureParameteri(texture, GL_TEXTURE_COMPARE_MODE, glMode);
			checkError();
		}
		if (config.compareFunc() != CompareFunc::Max) {
			const GLenum glFunc = _priv::CompareFuncs[core::enumVal(config.compareFunc())];
			glTextureParameteri(texture, GL_TEXTURE_COMPARE_FUNC, glFunc);
			checkError();
		}
#ifndef __EMSCRIPTEN__
		if (config.useBorderColor()) {
			glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(config.borderColor()));
		}
		if (config.lodBias() != 0.0f) {
			const GLfloat requested = config.lodBias();
			const GLfloat maxLodBias = limit(Limit::MaxLodBias);
			const GLfloat clamped = (maxLodBias > 0.0f) ? glm::clamp(requested, -maxLodBias, maxLodBias) : requested;
			core_assert(glTextureParameterf != nullptr);
			glTextureParameterf(texture, GL_TEXTURE_LOD_BIAS, clamped);
			checkError();
		}
#endif
		/** Specifies the index of the lowest defined mipmap level. This is an integer value. The initial value is 0. */
		// glTextureParameteri(texture, GL_TEXTURE_BASE_LEVEL, 0);
		/** Sets the index of the highest defined mipmap level. This is an integer value. The initial value is 1000. */
		// glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);

		if (FLEXT_ARB_texture_filter_anisotropic) {
			const GLfloat maxAnisotropy = config.maxAnisotropy();
			if (maxAnisotropy > 1.0f) {
				const GLfloat limitMaxAnisotropy = limit(Limit::MaxAnisotropy);
				const GLfloat clampedMaxAnisotropy =
					core_min(maxAnisotropy, limitMaxAnisotropy > 0.0f ? limitMaxAnisotropy : maxAnisotropy);
				core_assert(glTextureParameterf != nullptr);
				glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, clampedMaxAnisotropy);
				checkError();
			}
		}
	} else {
		core_assert(glTexParameteri != nullptr);
		core_assert(glTexParameterfv != nullptr);
		if (config.type() != TextureType::Texture2DMultisample && config.filterMag() != TextureFilter::Max) {
			const GLenum glFilterMag = _priv::TextureFilters[core::enumVal(config.filterMag())];
			glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, glFilterMag);
			checkError();
		}
		if (config.type() != TextureType::Texture2DMultisample && config.filterMin() != TextureFilter::Max) {
			const GLenum glFilterMin = _priv::TextureFilters[core::enumVal(config.filterMin())];
			glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, glFilterMin); // TODO: RENDERER: mipmapping
			checkError();
		}
		if (config.type() == TextureType::Texture3D && config.wrapR() != TextureWrap::Max) {
			const GLenum glWrapR = _priv::TextureWraps[core::enumVal(config.wrapR())];
			glTexParameteri(glType, GL_TEXTURE_WRAP_R, glWrapR);
			checkError();
		}
		if ((config.type() == TextureType::Texture2D || config.type() == TextureType::Texture3D) &&
			config.wrapS() != TextureWrap::Max) {
			const GLenum glWrapS = _priv::TextureWraps[core::enumVal(config.wrapS())];
			glTexParameteri(glType, GL_TEXTURE_WRAP_S, glWrapS);
			checkError();
		}
		if ((config.type() == TextureType::Texture2D || config.type() == TextureType::Texture3D) &&
			config.wrapT() != TextureWrap::Max) {
			const GLenum glWrapT = _priv::TextureWraps[core::enumVal(config.wrapT())];
			glTexParameteri(glType, GL_TEXTURE_WRAP_T, glWrapT);
			checkError();
		}
		if (config.compareMode() != TextureCompareMode::Max) {
			const GLenum glMode = _priv::TextureCompareModes[core::enumVal(config.compareMode())];
			glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, glMode);
			checkError();
		}
		if (config.compareFunc() != CompareFunc::Max) {
			const GLenum glFunc = _priv::CompareFuncs[core::enumVal(config.compareFunc())];
			glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, glFunc);
			checkError();
		}
#ifndef __EMSCRIPTEN__
		if (config.useBorderColor()) {
			glTexParameterfv(glType, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(config.borderColor()));
		}
		if (config.lodBias() != 0.0f) {
			const GLfloat requested = config.lodBias();
			const GLfloat maxLodBias = limit(Limit::MaxLodBias);
			const GLfloat clamped = (maxLodBias > 0.0f) ? glm::clamp(requested, -maxLodBias, maxLodBias) : requested;
			core_assert(glTexParameterf != nullptr);
			glTexParameterf(glType, GL_TEXTURE_LOD_BIAS, clamped);
			checkError();
		}
#endif
		/** Specifies the index of the lowest defined mipmap level. This is an integer value. The initial value is 0. */
		// glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
		/** Sets the index of the highest defined mipmap level. This is an integer value. The initial value is 1000. */
		// glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);

		if (FLEXT_ARB_texture_filter_anisotropic) {
			const GLfloat maxAnisotropy = config.maxAnisotropy();
			if (maxAnisotropy > 1.0f) {
				const GLfloat limitMaxAnisotropy = limit(Limit::MaxAnisotropy);
				const GLfloat clampedMaxAnisotropy =
					core_min(maxAnisotropy, limitMaxAnisotropy > 0.0f ? limitMaxAnisotropy : maxAnisotropy);
				core_assert(glTexParameterf != nullptr);
				glTexParameterf(glType, GL_TEXTURE_MAX_ANISOTROPY, clampedMaxAnisotropy);
				checkError();
			}
		}
	}
	const uint8_t alignment = config.alignment();
	if (alignment > 0u) {
		core_assert(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
		core_assert(glPixelStorei != nullptr);
		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	}
	checkError();
}

void uploadTexture(Id texture, int width, int height, const uint8_t *data, int index, const TextureConfig &cfg) {
	video_trace_scoped(UploadTexture);
	const int samples = cfg.samples();
	const TextureType type = cfg.type();
	const TextureFormat format = cfg.format();
	const _priv::Formats &f = _priv::textureFormats[core::enumVal(format)];
	// Determine whether we should allocate mip levels and generate mipmaps.
	bool wantMipmaps = false;
	if (cfg.filterMin() != TextureFilter::Max) {
		const TextureFilter fm = cfg.filterMin();
		// If the min filter is one of the mipmap-aware modes, request mipmaps.
		if (fm == TextureFilter::NearestMipmapNearest || fm == TextureFilter::NearestMipmapLinear ||
			fm == TextureFilter::LinearMipmapNearest || fm == TextureFilter::LinearMipmapLinear) {
			wantMipmaps = true;
		}
	}
	// Multisample textures cannot have mipmaps
	if (type == TextureType::Texture2DMultisample || type == TextureType::Texture2DMultisampleArray) {
		wantMipmaps = false;
	}

	const int levels =
		wantMipmaps && width > 0 && height > 0 ? (int)glm::floor(glm::log2((float)core_max(width, height))) + 1 : 1;
	if (useFeature(Feature::DirectStateAccess)) {
		core_assert(glTextureStorage2D != nullptr);
		core_assert(glTextureStorage3D != nullptr);
		if (type == TextureType::Texture1D) {
			core_assert(height == 1);
			core_assert(glTextureStorage1D != nullptr);
			core_assert(glTextureSubImage1D != nullptr);
			glTextureStorage1D(texture, levels, f.internalFormat, width);
			checkError();
			if (data) {
				glTextureSubImage1D(texture, 0, 0, width, f.dataFormat, f.dataType, (const GLvoid *)data);
				checkError();
			}
		} else if (type == TextureType::Texture2D) {
			core_assert(glTextureStorage2D != nullptr);
			core_assert(glTextureSubImage2D != nullptr);
			glTextureStorage2D(texture, levels, f.internalFormat, width, height);
			checkError();
			if (data) {
				glTextureSubImage2D(texture, 0, 0, 0, width, height, f.dataFormat, f.dataType, (const GLvoid *)data);
				checkError();
			}
		} else if (type == TextureType::Texture2DMultisample) {
			core_assert(samples > 0);
			core_assert(glTextureStorage2D != nullptr);
			core_assert(glTextureSubImage2D != nullptr);
			glTextureStorage2DMultisample(texture, samples, f.internalFormat, width, height, false);
			checkError();
			if (data) {
				glTextureSubImage2D(texture, 0, 0, 0, width, height, f.dataFormat, f.dataType, (const GLvoid *)data);
				checkError();
			}
		} else if (type == TextureType::Texture2DMultisampleArray) {
			core_assert(samples > 0);
			core_assert(index > 0);
			core_assert(glTextureStorage3DMultisample != nullptr);
			core_assert(glTextureSubImage3D != nullptr);
			glTextureStorage3DMultisample(texture, samples, f.internalFormat, width, height, index, false);
			checkError();
			if (data) {
				glTextureSubImage3D(texture, 0, 0, 0, 0, width, height, index, f.dataFormat, f.dataType,
									(const GLvoid *)data);
				checkError();
			}
		} else {
			core_assert(glTextureStorage3D != nullptr);
			core_assert(glTextureSubImage3D != nullptr);
			glTextureStorage3D(texture, levels, f.internalFormat, width, height, index);
			checkError();
			if (data) {
				glTextureSubImage3D(texture, 0, 0, 0, 0, width, height, index, f.dataFormat, f.dataType,
									(const GLvoid *)data);
				checkError();
			}
		}
		if (wantMipmaps && levels > 1) {
			// Allocate storage already used levels above; generate mips on the GPU
			if (glGenerateTextureMipmap != nullptr) {
				glGenerateTextureMipmap(texture);
			}
			checkError();
		}
	} else {
		const GLenum glType = _priv::TextureTypes[core::enumVal(type)];
		core_assert(type != TextureType::Max);
		if (type == TextureType::Texture1D) {
			core_assert(height == 1);
			core_assert(glTexImage1D != nullptr);
			glTexImage1D(glType, 0, f.internalFormat, width, 0, f.dataFormat, f.dataType, (const GLvoid *)data);
		} else if (type == TextureType::Texture2D) {
			core_assert(glTexImage2D != nullptr);
			glTexImage2D(glType, 0, f.internalFormat, width, height, 0, f.dataFormat, f.dataType, (const GLvoid *)data);
			checkError();
		} else if (type == TextureType::Texture2DMultisample) {
			core_assert(samples > 0);
			core_assert(glTexImage2DMultisample != nullptr);
			glTexImage2DMultisample(glType, samples, f.internalFormat, width, height, false);
			checkError();
		} else if (type == TextureType::Texture2DMultisampleArray) {
			core_assert(glTexImage3DMultisample != nullptr);
			glTexImage3DMultisample(glType, samples, f.internalFormat, width, height, index, false);
			checkError();
		} else {
			core_assert(glTexImage3D != nullptr);
			glTexImage3D(glType, 0, f.internalFormat, width, height, index, 0, f.dataFormat, f.dataType,
						 (const GLvoid *)data);
			checkError();
		}
		if (wantMipmaps && levels > 1) {
			// generate on currently bound target
			core_assert(glGenerateMipmap != nullptr);
			glGenerateMipmap(glType);
			checkError();
		}
	}
}

void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset) {
	video_trace_scoped(DrawElements);
	if (numIndices <= 0) {
		return;
	}
	rendererState().drawCalls++;
	syncState();
	core_assert_msg(rendererState().vertexArrayHandle != InvalidId, "No vertex buffer is bound for this draw call");
	const GLenum glMode = _priv::Primitives[core::enumVal(mode)];
	const GLenum glType = _priv::DataTypes[core::enumVal(type)];
	video::validate(rendererState().programHandle);
	core_assert(glDrawElements != nullptr);
	glDrawElements(glMode, (GLsizei)numIndices, glType, (GLvoid *)offset);
	checkError();
}

void drawArrays(Primitive mode, size_t count) {
	if (count == 0u) {
		return;
	}
	video_trace_scoped(DrawArrays);
	rendererState().drawCalls++;
	syncState();
	const GLenum glMode = _priv::Primitives[core::enumVal(mode)];
	video::validate(rendererState().programHandle);
	core_assert(glDrawArrays != nullptr);
	glDrawArrays(glMode, (GLint)0, (GLsizei)count);
	checkError();
}

void enableDebug(DebugSeverity severity) {
	if (severity == DebugSeverity::None) {
		return;
	}
	if (!useFeature(Feature::DebugOutput)) {
		Log::warn("No debug feature support was detected");
		return;
	}
	GLenum glSeverity = GL_DONT_CARE;
	switch (severity) {
	case DebugSeverity::High:
		glSeverity = GL_DEBUG_SEVERITY_HIGH_ARB;
		break;
	case DebugSeverity::Medium:
		glSeverity = GL_DEBUG_SEVERITY_MEDIUM_ARB;
		break;
	default:
	case DebugSeverity::Low:
		glSeverity = GL_DEBUG_SEVERITY_LOW_ARB;
		break;
	}

	core_assert(glDebugMessageControlARB != nullptr);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
	enable(State::DebugOutput);
	core_assert(glDebugMessageCallbackARB != nullptr);
	glDebugMessageCallbackARB(_priv::debugOutputCallback, nullptr);
	checkError();
	Log::info("enable opengl debug messages");
}

bool compileShader(Id id, ShaderType shaderType, const core::String &source, const core::String &name) {
	video_trace_scoped(CompileShader);
	if (id == InvalidId) {
		return false;
	}
	const char *src = source.c_str();
	video::checkError();
	const GLuint lid = (GLuint)id;
	core_assert(glShaderSource != nullptr);
	glShaderSource(lid, 1, (const GLchar **)&src, nullptr);
	video::checkError();
	core_assert(glCompileShader != nullptr);
	glCompileShader(lid);
	video::checkError();

	GLint status = 0;
	core_assert(glGetShaderiv != nullptr);
	glGetShaderiv(lid, GL_COMPILE_STATUS, &status);
	video::checkError();
	if (status == GL_TRUE) {
		return true;
	}
	GLint infoLogLength = 0;
	core_assert(glGetShaderiv != nullptr);
	glGetShaderiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);
	video::checkError();

	if (infoLogLength > 1) {
		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		core_assert(glGetShaderInfoLog != nullptr);
		glGetShaderInfoLog(lid, infoLogLength, nullptr, strInfoLog);
		video::checkError();
		const core::String compileLog(strInfoLog, static_cast<size_t>(infoLogLength));
		delete[] strInfoLog;
		const char *strShaderType;
		switch (shaderType) {
		case ShaderType::Vertex:
			strShaderType = "vertex";
			break;
		case ShaderType::Fragment:
			strShaderType = "fragment";
			break;
		case ShaderType::Geometry:
			strShaderType = "geometry";
			break;
		case ShaderType::Compute:
			strShaderType = "compute";
			break;
		default:
			strShaderType = "unknown";
			break;
		}

		if (status != GL_TRUE) {
			Log::error("Failed to compile: %s\n%s\nshaderType: %s", name.c_str(), compileLog.c_str(), strShaderType);
			core::DynamicArray<core::String> tokens;
			core::string::splitString(source, tokens, "\n");
			int i = 1;
			for (const core::String &line : tokens) {
				Log::error("%03i: %s", i, line.c_str());
				++i;
			}
		} else {
			Log::info("%s: %s", name.c_str(), compileLog.c_str());
		}
	}
	deleteShader(id);
	return false;
}

bool linkComputeShader(Id program, Id comp, const core::String &name) {
	video_trace_scoped(LinkComputeShader);
	const GLuint lid = (GLuint)program;
	core_assert(glAttachShader != nullptr);
	glAttachShader(lid, comp);
	video::checkError();
	core_assert(glLinkProgram != nullptr);
	glLinkProgram(lid);
	GLint status = 0;
	core_assert(glGetProgramiv != nullptr);
	glGetProgramiv(lid, GL_LINK_STATUS, &status);
	video::checkError();
	if (status == GL_FALSE) {
		GLint infoLogLength = 0;
		core_assert(glGetProgramiv != nullptr);
		glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);
		video::checkError();

		if (infoLogLength > 1) {
			GLchar *strInfoLog = new GLchar[infoLogLength + 1];
			core_assert(glGetProgramInfoLog != nullptr);
			glGetProgramInfoLog(lid, infoLogLength, nullptr, strInfoLog);
			video::checkError();
			const core::String linkLog(strInfoLog, static_cast<size_t>(infoLogLength));
			if (status != GL_TRUE) {
				Log::error("Failed to link: %s\n%s", name.c_str(), linkLog.c_str());
			} else {
				Log::info("%s: %s", name.c_str(), linkLog.c_str());
			}
			delete[] strInfoLog;
		}
	}
	core_assert(glDetachShader != nullptr);
	glDetachShader(lid, comp);
	video::checkError();
	if (status != GL_TRUE) {
		deleteProgram(program);
		return false;
	}

	return true;
}

bool bindImage(Id textureHandle, AccessMode mode, ImageFormat format) {
	if (rendererState().imageHandle == textureHandle && rendererState().imageFormat == format &&
		rendererState().imageAccessMode == mode) {
		return false;
	}
	const GLenum glFormat = _priv::ImageFormatTypes[core::enumVal(format)];
	const GLenum glAccessMode = _priv::AccessModes[core::enumVal(mode)];
	const GLuint unit = 0u;
	const GLint level = 0;
	const GLboolean layered = GL_FALSE;
	const GLint layer = 0;
	core_assert(glBindImageTexture != nullptr);
	glBindImageTexture(unit, (GLuint)textureHandle, level, layered, layer, glAccessMode, glFormat);
	video::checkError();
	/* update cached binding so redundant binds can be avoided */
	rendererState().imageHandle = textureHandle;
	rendererState().imageFormat = format;
	rendererState().imageAccessMode = mode;
	return true;
}

void waitShader(MemoryBarrierType wait) {
	video_trace_scoped(WaitShader);
	if (wait == MemoryBarrierType::None || glMemoryBarrier == nullptr) {
		return;
	}
	const GLenum glBarrier = _priv::MemoryBarrierTypes[core::enumVal(wait)];
	core_assert(glMemoryBarrier != nullptr);
	glMemoryBarrier(glBarrier);
	video::checkError();
}

bool runShader(Id program, const glm::uvec3 &workGroups, MemoryBarrierType wait) {
	video_trace_scoped(RunShader);
	if (workGroups.x <= 0 || workGroups.y <= 0 || workGroups.z <= 0) {
		return false;
	}
	if (!checkLimit(workGroups.x, Limit::MaxComputeWorkGroupCountX)) {
		return false;
	}
	if (!checkLimit(workGroups.y, Limit::MaxComputeWorkGroupCountY)) {
		return false;
	}
	if (!checkLimit(workGroups.z, Limit::MaxComputeWorkGroupCountZ)) {
		return false;
	}

	video::validate(program);
	core_assert(glDispatchCompute != nullptr);
	glDispatchCompute((GLuint)workGroups.x, (GLuint)workGroups.y, (GLuint)workGroups.z);
	video::checkError();
	waitShader(wait);
	return true;
}

bool linkShader(Id program, Id vert, Id frag, Id geom, const core::String &name) {
	video_trace_scoped(LinkShader);
	const GLuint lid = (GLuint)program;
	core_assert(glAttachShader != nullptr);
	glAttachShader(lid, (GLuint)vert);
	checkError();
	glAttachShader(lid, (GLuint)frag);
	checkError();
	if (geom != InvalidId) {
		glAttachShader(lid, (GLuint)geom);
		checkError();
	}

	core_assert(glLinkProgram != nullptr);
	glLinkProgram(lid);
	checkError();
	GLint status = 0;
	core_assert(glGetProgramiv != nullptr);
	glGetProgramiv(lid, GL_LINK_STATUS, &status);
	checkError();
	if (status == GL_FALSE) {
		GLint infoLogLength = 0;
		glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);
		checkError();

		if (infoLogLength > 1) {
			GLchar *strInfoLog = new GLchar[infoLogLength + 1];
			core_assert(glGetProgramInfoLog != nullptr);
			glGetProgramInfoLog(lid, infoLogLength, nullptr, strInfoLog);
			checkError();
			const core::String linkLog(strInfoLog, static_cast<size_t>(infoLogLength));
			if (status != GL_TRUE) {
				Log::error("Failed to link: %s\n%s", name.c_str(), linkLog.c_str());
			} else {
				Log::info("%s: %s", name.c_str(), linkLog.c_str());
			}
			delete[] strInfoLog;
		}
	}
	core_assert(glDetachShader != nullptr);
	glDetachShader(lid, (GLuint)vert);
	video::checkError();
	glDetachShader(lid, (GLuint)frag);
	video::checkError();
	if (geom != InvalidId) {
		glDetachShader(lid, (GLuint)geom);
		video::checkError();
	}
	if (status != GL_TRUE) {
		deleteProgram(program);
		return false;
	}

	return true;
}

int fetchUniforms(Id program, ShaderUniforms &uniforms, const core::String &name) {
	video_trace_scoped(FetchUniforms);
	int uniformsCnt = _priv::fillUniforms(program, uniforms, name, false);
	int uniformBlocksCnt = _priv::fillUniforms(program, uniforms, name, true);

	if (limiti(Limit::MaxUniformBufferSize) > 0) {
		for (auto *e : uniforms) {
			if (!e->value.block) {
				continue;
			}
			if (e->value.size > limiti(Limit::MaxUniformBufferSize)) {
				Log::error("Max uniform buffer size exceeded for uniform %s at location %i (max is %i)", e->key.c_str(),
						   e->value.location, limiti(Limit::MaxUniformBufferSize));
			} else if (e->value.size <= 0) {
				Log::error("Failed to query size of uniform buffer %s at location %i (max is %i)", e->key.c_str(),
						   e->value.location, limiti(Limit::MaxUniformBufferSize));
			}
		}
	}
	return uniformsCnt + uniformBlocksCnt;
}

int fetchAttributes(Id program, ShaderAttributes &attributes, const core::String &name) {
	video_trace_scoped(FetchAttributes);
	char varName[MAX_SHADER_VAR_NAME];
	int numAttributes = 0;
	const GLuint lid = (GLuint)program;
	core_assert(glGetProgramiv != nullptr);
	glGetProgramiv(lid, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	checkError();

	for (int i = 0; i < numAttributes; ++i) {
		GLsizei length;
		GLint size;
		GLenum type;
		core_assert(glGetActiveAttrib != nullptr);
		glGetActiveAttrib(lid, i, MAX_SHADER_VAR_NAME - 1, &length, &size, &type, varName);
		video::checkError();
		core_assert(glGetAttribLocation != nullptr);
		const int location = glGetAttribLocation(lid, varName);
		attributes.put(varName, location);
		Log::debug("attribute location for %s is %i (shader %s)", varName, location, name.c_str());
	}
	return numAttributes;
}

void destroyContext(RendererContext &context) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_GL_DestroyContext((SDL_GLContext)context);
#else
	SDL_GL_DeleteContext((SDL_GLContext)context);
#endif
}

RendererContext createContext(SDL_Window *window) {
	core_assert(window != nullptr);
	Log::debug("Trying to create an opengl context");
	return (RendererContext)SDL_GL_CreateContext(window);
}

void activateContext(SDL_Window *window, RendererContext &context) {
	SDL_GL_MakeCurrent(window, (SDL_GLContext)context);
}

void endFrame(SDL_Window *window) {
	SDL_GL_SwapWindow(window);
}

void setup() {
	SDL_ClearError();
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
#ifdef USE_OPENGLES
	int contextFlags = 0;
	GLVersion glv = GLES3;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	int contextFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
	const core::VarPtr &glVersion = core::Var::getSafe(cfg::ClientOpenGLVersion);
	int glMinor = 0, glMajor = 0;
	if (SDL_sscanf(glVersion->strVal().c_str(), "%3i.%3i", &glMajor, &glMinor) != 2) {
		const GLVersion &version = GL4_3;
		glMajor = version.majorVersion;
		glMinor = version.minorVersion;
	}
	GLVersion glv(glMajor, glMinor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	const core::VarPtr &multisampleBuffers = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	const core::VarPtr &multisampleSamples = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	int samples = multisampleSamples->intVal();
	int buffers = multisampleBuffers->intVal();
	if (samples <= 0) {
		buffers = 0;
	} else if (buffers <= 0) {
		samples = 0;
	}
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	Log::debug("Request gles context %i.%i", glv.majorVersion, glv.minorVersion);
	for (size_t i = 0; i < lengthof(GLVersions); ++i) {
		if (GLVersions[i].version == glv) {
			Shader::glslVersion = GLVersions[i].glslVersion;
			break;
		}
	}
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glv.majorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glv.minorVersion);
#ifdef DEBUG
	contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
	Log::debug("Enable opengl debug context");
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
}

static bool setVSync(int value) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_GL_SetSwapInterval(value);
#else
	return SDL_GL_SetSwapInterval(value) != -1;
#endif
}

static int getVSync() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	int val = 0;
	SDL_GL_GetSwapInterval(&val);
	return val;
#else
	return SDL_GL_GetSwapInterval();
#endif
}

static void handleVSync() {
	const bool vsync = core::Var::getSafe(cfg::ClientVSync)->boolVal();
	if (vsync) {
		if (!setVSync(-1)) {
			if (!setVSync(1)) {
				Log::warn("Could not activate vsync: %s", SDL_GetError());
			}
		}
	} else {
		setVSync(0);
	}
	if (getVSync() == 0) {
		Log::debug("Deactivated vsync");
	} else {
		Log::debug("Activated vsync");
	}
}

bool init(int windowWidth, int windowHeight, float scaleFactor) {
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &glState().glVersion.majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &glState().glVersion.minorVersion);
	Log::debug("got gl context: %i.%i", glState().glVersion.majorVersion, glState().glVersion.minorVersion);
	RendererState &rs = rendererState();

	resize(windowWidth, windowHeight, scaleFactor);

	if (flextInit() == -1) {
		Log::error("Could not initialize opengl: %s", SDL_GetError());
		return false;
	}

	_priv::setupFeatures(glState().glVersion);
	_priv::setupLimitsAndSpecs();

	const char *glvendor = (const char *)glGetString(GL_VENDOR);
	const char *glrenderer = (const char *)glGetString(GL_RENDERER);
	const char *glversion = (const char *)glGetString(GL_VERSION);
	Log::debug("GL_VENDOR: %s", glvendor);
	Log::debug("GL_RENDERER: %s", glrenderer);
	Log::debug("GL_VERSION: %s", glversion);
	if (glvendor != nullptr) {
		const core::String vendor(glvendor);
		for (int i = 0; i < core::enumVal(Vendor::Max); ++i) {
			const bool match = core::string::icontains(vendor, _priv::VendorStrings[i]);
			rs.vendor.set(i, match);
		}
	}

	for (int i = 0; i < core::enumVal(Vendor::Max); ++i) {
		if (rs.vendor[i]) {
			Log::debug("Found vendor: %s", _priv::VendorStrings[i]);
		} else {
			Log::debug("Didn't find vendor: %s", _priv::VendorStrings[i]);
		}
	}

	handleVSync();

	if (useFeature(Feature::DirectStateAccess)) {
		Log::debug("Use direct state access");
	} else {
		Log::debug("No direct state access");
	}

	int contextFlags = 0;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
	if (contextFlags & SDL_GL_CONTEXT_DEBUG_FLAG) {
		const int severity = core::Var::getSafe(cfg::ClientDebugSeverity)->intVal();
		if (severity < (int)video::DebugSeverity::None || severity >= (int)video::DebugSeverity::Max) {
			Log::warn("Invalid severity level given: %i [0-3] - 0 disabled, 1 highest and 3 lowest severity level",
					  severity);
		} else {
			enableDebug((video::DebugSeverity)severity);
		}
	}

	const core::VarPtr &multisampleBuffers = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	const core::VarPtr &multisampleSamples = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	bool multisampling = multisampleSamples->intVal() > 0 && multisampleBuffers->intVal() > 0;
	if (multisampling) {
		int buffers, samples;
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
		SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
		if (buffers == 0 || samples == 0) {
			Log::warn("Could not get FSAA context");
			multisampling = false;
		} else {
			Log::debug("Got FSAA context with %i buffers and %i samples", buffers, samples);
		}
	}

	int profile;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);
	if (profile == SDL_GL_CONTEXT_PROFILE_CORE) {
		Log::debug("Got core profile");
	} else if (profile == SDL_GL_CONTEXT_PROFILE_ES) {
		Log::debug("Got ES profile");
	} else if (profile == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
		Log::debug("Got compatibility profile");
	} else {
		Log::warn("Unknown profile: %i", profile);
	}

	// default state
	// https://www.glprogramming.com/red/appendixb.html
	rs.states.set(core::enumVal(video::State::DepthMask), true);
	glGetFloatv(GL_POINT_SIZE, &rs.pointSize);
	if (rs.smoothedLineWidth.x < 0.0f) {
#ifdef USE_OPENGLES
		GLfloat buf[2];
		core_assert(glGetFloatv != nullptr);
		glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		rs.smoothedLineWidth.x = buf[0];
		rs.smoothedLineWidth.y = buf[1];
		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, buf);
		rs.aliasedLineWidth.x = buf[0];
		rs.aliasedLineWidth.y = buf[1];
#else
		GLdouble buf[2];
		core_assert(glGetDoublev != nullptr);
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		rs.smoothedLineWidth.x = (float)buf[0];
		rs.smoothedLineWidth.y = (float)buf[1];
		glGetDoublev(GL_ALIASED_LINE_WIDTH_RANGE, buf);
		rs.aliasedLineWidth.x = (float)buf[0];
		rs.aliasedLineWidth.y = (float)buf[1];
#endif
		// TODO GL_SMOOTH_LINE_WIDTH_GRANULARITY
	}
	if (multisampling) {
		video::enable(video::State::MultiSample);
	}

	// set some default values
	blendEquation(BlendEquation::Add);
	blendFuncSeparate(BlendMode::SourceAlpha, BlendMode::OneMinusSourceAlpha, BlendMode::One,
					  BlendMode::OneMinusSourceAlpha);

	return true;
}

void setUniformBufferBinding(Id program, uint32_t blockIndex, uint32_t blockBinding) {
	core_assert(glUniformBlockBinding != nullptr);
	// Use a combined key: (program << 32 | blockIndex)
	const uint64_t key = (static_cast<uint64_t>(program) << 32) | blockIndex;
	uint32_t cachedBinding = 0;
	if (rendererState().uniformBufferBindings.get(key, cachedBinding)) {
		if (cachedBinding == blockBinding) {
			return; // binding already set, avoid redundant call
		}
	}
	glUniformBlockBinding(program, (GLuint)blockIndex, (GLuint)blockBinding);
	checkError();
	rendererState().uniformBufferBindings.put(key, blockBinding);
}

int32_t getUniformBufferOffset(Id program, const char *name) {
	GLuint index;
	const GLchar *uniformNames[1];
	uniformNames[0] = name;
	core_assert(glGetUniformIndices != nullptr);
	glGetUniformIndices(program, 1, uniformNames, &index);
	checkError();
	if (index == GL_INVALID_INDEX) {
		Log::error("Could not query uniform index for %s", name);
		return -1;
	}
	GLint offset;
	core_assert(glGetActiveUniformsiv != nullptr);
	glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &offset);
	checkError();
	GLint type;
	glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_TYPE, &type);
	checkError();
	GLint size;
	glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_SIZE, &size); // array length, not actual type size;
	checkError();
	GLint matrixStride;
	glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_MATRIX_STRIDE, &matrixStride);
	checkError();
	GLint arrayStride;
	glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_ARRAY_STRIDE, &arrayStride);
	checkError();
	Log::debug("%s: offset: %i, type: %i, size: %i, matrixStride: %i, arrayStride: %i", name, offset, type, size, matrixStride, arrayStride);
	return offset;
}

void traceVideoBegin(const char *name) {
	if (glPushDebugGroup == nullptr)
		return;
	// glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
	// checkError();
}

void traceVideoEnd() {
	if (glPopDebugGroup == nullptr)
		return;
	// glPopDebugGroup();
}

} // namespace video
