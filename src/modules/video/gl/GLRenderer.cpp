/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "GLRenderer.h"
#include "GLHelper.h"
#include "GLMapping.h"
#include "GLState.h"
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

bool useProgram(Id handle) {
	if (rendererState().programHandle == handle) {
		return false;
	}
	core_assert(handle == InvalidId || glIsProgram(handle));
	core_assert(glUseProgram != nullptr);
	glUseProgram(handle);
	checkError();
	rendererState().programHandle = handle;
	rendererState().needValidation = true;
	return true;
}

Id getProgram() {
	return rendererState().programHandle;
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

Id boundVertexArray() {
	return rendererState().vertexArrayHandle;
}

Id boundBuffer(BufferType type) {
	const int typeIndex = core::enumVal(type);
	return rendererState().bufferHandle[typeIndex];
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
	if (rendererState().bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	rendererState().bufferHandle[typeIndex] = handle;
	core_assert(glBindBufferBase != nullptr);
	glBindBufferBase(glType, (GLuint)index, handle);
	checkError();
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

void deleteVertexArray(Id &id) {
	if (id == InvalidId) {
		return;
	}
	if (rendererState().vertexArrayHandle == id) {
		bindVertexArray(InvalidId);
	}
	deleteVertexArrays(1, &id);
	id = InvalidId;
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

IdPtr genFenc() {
	return (IdPtr)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void deleteFence(IdPtr &id) {
	if (id == InvalidIdPtr) {
		return;
	}
	static_assert(sizeof(IdPtr) >= sizeof(GLsync), "Unexpected sizes");
	glDeleteSync((GLsync)id);
	id = InvalidId;
}

bool checkFence(IdPtr id, uint64_t timeout) {
	if (id == InvalidIdPtr) {
		return false;
	}
	if (timeout == 0) {
		return false;
	}
	static_assert(sizeof(IdPtr) >= sizeof(GLsync), "Unexpected sizes");
#if SANITY_CHECKS_GL
	if (!glIsSync((GLsync)id)) {
		return false;
	}
#endif
	const int retVal = glClientWaitSync((GLsync)id, GL_SYNC_FLUSH_COMMANDS_BIT, (GLuint64)timeout);
	checkError();
	if (retVal == GL_CONDITION_SATISFIED || retVal == GL_ALREADY_SIGNALED) {
		return true;
	}
	// if (retVal == GL_TIMEOUT_EXPIRED) {
	// 	return false;
	// }
	return false;
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

Id currentFramebuffer() {
	return rendererState().framebufferHandle;
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

void configureAttribute(const Attribute &a) {
	video_trace_scoped(ConfigureVertexAttribute);
	core_assert(rendererState().programHandle != InvalidId);
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
	video_trace_scoped(DrawArrays);
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

void startFrame(SDL_Window *window, RendererContext &context) {
	activateContext(window, context);
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

void resize(int windowWidth, int windowHeight, float scaleFactor) {
	rendererState().windowWidth = windowWidth;
	rendererState().windowHeight = windowHeight;
	rendererState().scaleFactor = scaleFactor;
}

glm::ivec2 getWindowSize() {
	return glm::ivec2(rendererState().windowWidth, rendererState().windowHeight);
}

float getScaleFactor() {
	return rendererState().scaleFactor;
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

void handleVSync() {
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

	resize(windowWidth, windowHeight, scaleFactor);

	if (flextInit() == -1) {
		Log::error("Could not initialize opengl: %s", SDL_GetError());
		return false;
	}

	_priv::setupFeatures();
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
			rendererState().vendor.set(i, match);
		}
	}

	for (int i = 0; i < core::enumVal(Vendor::Max); ++i) {
		if (rendererState().vendor[i]) {
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
	rendererState().states.set(core::enumVal(video::State::DepthMask), true);
	glGetFloatv(GL_POINT_SIZE, &rendererState().pointSize);

	if (multisampling) {
		video::enable(video::State::MultiSample);
	}

	// set some default values
	blendEquation(BlendEquation::Add);
	blendFuncSeparate(BlendMode::SourceAlpha, BlendMode::OneMinusSourceAlpha, BlendMode::One,
					  BlendMode::OneMinusSourceAlpha);

	return true;
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
