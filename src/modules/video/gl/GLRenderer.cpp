/**
 * @file
 *
 * Some great tips here: https://developer.nvidia.com/opengl-vulkan
 */

#include "GLRenderer.h"
#include "video/Renderer.h"
#include "GLTypes.h"
#include "GLState.h"
#include "GLMapping.h"
#include "GLHelper.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <SDL.h>
#include <map>

namespace video {

static RenderState s;

#ifndef MAX_SHADER_VAR_NAME
#define MAX_SHADER_VAR_NAME 128
#endif

#define SANITY_CHECKS_GL 0
#define DIRECT_STATE_ACCESS 0

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
		}
		hasError |= (glError == GL_NO_ERROR);
	}
	return hasError;
#else
	return false;
#endif
}

//TODO: use FrameBufferConfig
void readBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	checkError();
}

//TODO: use FrameBufferConfig
bool setupGBuffer(Id fbo, const glm::ivec2& dimension, Id* textures, size_t texCount, Id depthTexture) {
	const Id prev = bindFramebuffer(fbo);

	TextureConfig cfg;
	// we are going to write vec3 into the out vars in the shaders
	cfg.format(TextureFormat::RGB32F).filter(TextureFilter::Nearest);
	for (std::size_t i = 0; i < texCount; ++i) {
		bindTexture(TextureUnit::Upload, cfg.type(), textures[i]);
		setupTexture(cfg);
		static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, (GLuint)textures[i], 0);
	}

	bindTexture(TextureUnit::Upload, TextureType::Texture2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)depthTexture, 0);

	core_assert(texCount == GBUFFER_NUM_TEXTURES);
	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);

	const bool retVal = _priv::checkFramebufferStatus();
	bindFramebuffer(prev);
	return retVal;
}

bool setupCubemap(Id handle, const image::ImagePtr images[6]) {
	bindTexture(TextureUnit::Upload, TextureType::TextureCube, handle);

	static const GLenum types[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	for (unsigned int i = 0; i < 6; ++i) {
		const image::ImagePtr& img = images[i];
		const GLenum mode = img->depth() == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(types[i], 0, mode, img->width(), img->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data());
	}

	// TODO: use setupTexture
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return true;
}

float lineWidth(float width) {
	if (_priv::s.smoothedLineWidth.x < 0.0f) {
		GLdouble buf[2];
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		_priv::s.smoothedLineWidth.x = (float)buf[0];
		_priv::s.smoothedLineWidth.y = (float)buf[1];
		glGetDoublev(GL_ALIASED_LINE_WIDTH_RANGE, buf);
		_priv::s.aliasedLineWidth.x = (float)buf[0];
		_priv::s.aliasedLineWidth.y = (float)buf[1];
		// TODO GL_SMOOTH_LINE_WIDTH_GRANULARITY
	}
	if (glm::abs(_priv::s.lineWidth - width) < glm::epsilon<float>()) {
		return _priv::s.lineWidth;
	}
	const float oldWidth = _priv::s.lineWidth;
	if (_priv::s.states[std::enum_value(State::LineSmooth)]) {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.smoothedLineWidth.x, _priv::s.smoothedLineWidth.y));
		checkError(false);
	} else {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.aliasedLineWidth.x, _priv::s.aliasedLineWidth.y));
		checkError(false);
	}
	_priv::s.lineWidth = width;
	return oldWidth;
}

bool clearColor(const glm::vec4& clearColor) {
	if (_priv::s.clearColor == clearColor) {
		return false;
	}
	_priv::s.clearColor = clearColor;
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	checkError();
	return true;
}

void clear(ClearFlag flag) {
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
	if (glValue == 0) {
		return;
	}
	// intel told me so... 5% performance gain if clear is called with disabled scissors.
	const bool enabled = disable(State::Scissor);
	glClear(glValue);
	if (enabled) {
		enable(State::Scissor);
	}
	checkError();
}

bool viewport(int x, int y, int w, int h) {
	if (_priv::s.viewportX == x && _priv::s.viewportY == y && _priv::s.viewportW == w && _priv::s.viewportH == h) {
		return false;
	}
	_priv::s.viewportX = x;
	_priv::s.viewportY = y;
	_priv::s.viewportW = w;
	_priv::s.viewportH = h;
	/**
	 * Parameters
	 * x, y
	 *  Specify the lower left corner of the viewport rectangle,
	 *  in pixels. The initial value is (0,0).
	 * width, height
	 *  Specify the width and height of the viewport.
	 *  When a GL context is first attached to a window,
	 *  width and height are set to the dimensions of that window.
	 *
	 * Description
	 *  glViewport specifies the affine transformation of x
	 *  and y from normalized device coordinates to window coordinates.
	 *
	 *  Viewport width and height are silently clamped
	 *  to a range that depends on the implementation.
	 *  To query this range, call glGet with argument
	 *  GL_MAX_VIEWPORT_DIMS.
	 */
	glViewport((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	checkError();
	return true;
}

void getViewport(int& x, int& y, int& w, int& h) {
	x = _priv::s.viewportX;
	y = _priv::s.viewportY;
	w = _priv::s.viewportW;
	h = _priv::s.viewportH;
}

void getScissor(int& x, int& y, int& w, int& h) {
	x = _priv::s.scissorX;
	y = _priv::s.scissorY;
	w = _priv::s.scissorW;
	h = _priv::s.scissorH;
}

bool scissor(int x, int y, int w, int h) {
	if (w < 0) {
		w = 0;
	}
	if (h < 0) {
		h = 0;
	}

	if (_priv::s.scissorX == x && _priv::s.scissorY == y && _priv::s.scissorW == w && _priv::s.scissorH == h) {
		return false;
	}
	_priv::s.scissorX = x;
	_priv::s.scissorY = y;
	_priv::s.scissorW = w;
	_priv::s.scissorH = h;
	/**
	 * Parameters
	 * x, y
	 *  Specify the lower left corner of the scissor box.
	 *  Initially (0, 0).
	 * width, height
	 *  Specify the width and height of the scissor box.
	 *  When a GL context is first attached to a window,
	 *  width and height are set to the dimensions of that
	 *  window.
	 *
	 * Description
	 *  glScissor defines a rectangle, called the scissor box,
	 *  in window coordinates.
	 *  The first two arguments,
	 *  x and y,
	 *  specify the lower left corner of the box.
	 *  width and height specify the width and height of the box.
	 *
	 *  To enable and disable the scissor test, call
	 *  glEnable and glDisable with argument
	 *  GL_SCISSOR_TEST. The test is initially disabled.
	 *  While the test is enabled, only pixels that lie within the scissor box
	 *  can be modified by drawing commands.
	 *  Window coordinates have integer values at the shared corners of
	 *  frame buffer pixels.
	 *  glScissor(0,0,1,1) allows modification of only the lower left
	 *  pixel in the window, and glScissor(0,0,0,0) doesn't allow
	 *  modification of any pixels in the window.
	 *
	 *  When the scissor test is disabled,
	 *  it is as though the scissor box includes the entire window.
	 */
	GLint bottom;
	if (_priv::s.clipOriginLowerLeft) {
		bottom = _priv::s.windowHeight - (_priv::s.scissorY + _priv::s.scissorH);
	} else {
		bottom = _priv::s.scissorY;
	}
	bottom = (GLint)glm::round(bottom * _priv::s.scaleFactor);
	const GLint left = (GLint)glm::round(_priv::s.scissorX * _priv::s.scaleFactor);
	const GLsizei width = (GLsizei)glm::round(_priv::s.scissorW * _priv::s.scaleFactor);
	const GLsizei height = (GLsizei)glm::round(_priv::s.scissorH * _priv::s.scaleFactor);
	glScissor(left, bottom, width, height);
	checkError();
	return true;
}

void colorMask(bool red, bool green, bool blue, bool alpha) {
	glColorMask((GLboolean)red, (GLboolean)green, (GLboolean)blue, (GLboolean)alpha);
	checkError();
}

bool enable(State state) {
	const int stateIndex = std::enum_value(state);
	if (_priv::s.states[stateIndex]) {
		return true;
	}
	_priv::s.states[stateIndex] = true;
	if (state == State::DepthMask) {
		glDepthMask(GL_TRUE);
	} else {
		glEnable(_priv::States[stateIndex]);
	}
	checkError();
	return false;
}

bool disable(State state) {
	const int stateIndex = std::enum_value(state);
	if (!_priv::s.states[stateIndex]) {
		return false;
	}
	_priv::s.states[stateIndex] = false;
	if (state == State::DepthMask) {
		glDepthMask(GL_FALSE);
	} else {
		glDisable(_priv::States[stateIndex]);
	}
	checkError();
	return true;
}

bool cullFace(Face face) {
	if (_priv::s.cullFace == face) {
		return false;
	}
	const GLenum glFace = _priv::Faces[std::enum_value(face)];
	glCullFace(glFace);
	checkError();
	_priv::s.cullFace = face;
	return true;
}

bool depthFunc(CompareFunc func) {
	if (_priv::s.depthFunc == func) {
		return false;
	}
	glDepthFunc(_priv::CompareFuncs[std::enum_value(func)]);
	checkError();
	_priv::s.depthFunc = func;
	return true;
}

bool setupStencil(const StencilConfig& config) {
	bool dirty = false;
	CompareFunc func = config.func();
	if (_priv::s.stencilFunc != func || _priv::s.stencilValue != config.value() || _priv::s.stencilMask != config.mask()) {
		glStencilFunc(_priv::CompareFuncs[std::enum_value(func)], config.value(), config.mask());
		checkError();
		_priv::s.stencilFunc = func;
		dirty = true;
	}
	// fail, zfail, zpass
	if (_priv::s.stencilOpFail != config.failOp()
			|| _priv::s.stencilOpZfail != config.zfailOp()
			|| _priv::s.stencilOpZpass != config.zpassOp()) {
		const GLenum failop = _priv::StencilOps[std::enum_value(config.failOp())];
		const GLenum zfailop = _priv::StencilOps[std::enum_value(config.zfailOp())];
		const GLenum zpassop = _priv::StencilOps[std::enum_value(config.zpassOp())];
		glStencilOp(failop, zfailop, zpassop);
		checkError();
		_priv::s.stencilOpFail = config.failOp();
		_priv::s.stencilOpZfail = config.zfailOp();
		_priv::s.stencilOpZpass = config.zpassOp();
		dirty = true;
	}
	if (_priv::s.stencilMask != config.mask()) {
		glStencilMask(config.mask());
		_priv::s.stencilMask = config.mask();
		dirty = true;
	}
	return dirty;
}

bool blendEquation(BlendEquation func) {
	if (_priv::s.blendEquation == func) {
		return false;
	}
	_priv::s.blendEquation = func;
	const GLenum convertedFunc = _priv::BlendEquations[std::enum_value(func)];
	glBlendEquation(convertedFunc);
	checkError();
	return true;
}

void getBlendState(bool& enabled, BlendMode& src, BlendMode& dest, BlendEquation& func) {
	const int stateIndex = std::enum_value(State::Blend);
	enabled = _priv::s.states[stateIndex];
	src = _priv::s.blendSrc;
	dest = _priv::s.blendDest;
	func = _priv::s.blendEquation;
}

bool blendFunc(BlendMode src, BlendMode dest) {
	if (_priv::s.blendSrc == src && _priv::s.blendDest == dest) {
		return false;
	}
	_priv::s.blendSrc = src;
	_priv::s.blendDest = dest;
	const GLenum glSrc = _priv::BlendModes[std::enum_value(src)];
	const GLenum glDest = _priv::BlendModes[std::enum_value(dest)];
	glBlendFunc(glSrc, glDest);
	checkError();
	return true;
}

PolygonMode polygonMode(Face face, PolygonMode mode) {
	if (_priv::s.polygonModeFace == face && _priv::s.polygonMode == mode) {
		return _priv::s.polygonMode;
	}
	_priv::s.polygonModeFace = face;
	const PolygonMode old = _priv::s.polygonMode;
	_priv::s.polygonMode = mode;
	const GLenum glMode = _priv::PolygonModes[std::enum_value(mode)];
	const GLenum glFace = _priv::Faces[std::enum_value(face)];
	glPolygonMode(glFace, glMode);
	checkError();
	return old;
}

bool polygonOffset(const glm::vec2& offset) {
	if (_priv::s.polygonOffset == offset) {
		return false;
	}
	glPolygonOffset(offset.x, offset.y);
	checkError();
	_priv::s.polygonOffset = offset;
	return true;
}

bool activateTextureUnit(TextureUnit unit) {
	if (_priv::s.textureUnit == unit) {
		return false;
	}
	core_assert(TextureUnit::Max != unit);
	const GLenum glUnit = _priv::TextureUnits[std::enum_value(unit)];
	glActiveTexture(glUnit);
	checkError();
	_priv::s.textureUnit = unit;
	return true;
}

Id currentTexture(TextureUnit unit) {
	if (TextureUnit::Max == unit) {
		return InvalidId;
	}
	return _priv::s.textureHandle[std::enum_value(unit)];
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	core_assert(TextureUnit::Max != unit);
	core_assert(TextureType::Max != type);
	const bool changeUnit = activateTextureUnit(unit);
	if (changeUnit || _priv::s.textureHandle[std::enum_value(unit)] != handle) {
		_priv::s.textureHandle[std::enum_value(unit)] = handle;
		glBindTexture(_priv::TextureTypes[std::enum_value(type)], handle);
		checkError();
		return true;
	}
	return false;
}

bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels) {
	bindTexture(unit, type, handle);
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const int pitch = w * f.bits / 8;
	*pixels = (uint8_t*)SDL_malloc(h * pitch);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(_priv::TextureTypes[std::enum_value(type)], 0, f.dataFormat, f.dataType, (void*)*pixels);
	if (checkError()) {
		SDL_free(*pixels);
		*pixels = nullptr;
		return false;
	}
	return true;
}

bool useProgram(Id handle) {
	if (_priv::s.programHandle == handle) {
		return false;
	}
	glUseProgram(handle);
	checkError();
	_priv::s.programHandle = handle;
	return true;
}

Id getProgram() {
	return _priv::s.programHandle;
}

bool bindVertexArray(Id handle) {
	if (_priv::s.vertexArrayHandle == handle) {
		return false;
	}
	glBindVertexArray(handle);
	checkError();
	_priv::s.vertexArrayHandle = handle;
	return true;
}

Id boundVertexArray() {
	return _priv::s.vertexArrayHandle;
}

Id boundBuffer(BufferType type) {
	const int typeIndex = std::enum_value(type);
	return _priv::s.bufferHandle[typeIndex];
}

void* mapBuffer(Id handle, BufferType type, AccessMode mode) {
	const int modeIndex = std::enum_value(mode);
	const GLenum glMode = _priv::AccessModes[modeIndex];
	if (hasFeature(Feature::DirectStateAccess)) {
		void* data = glMapNamedBuffer(handle, glMode);
		checkError();
		return data;
	}
	bindBuffer(type, handle);
	const int typeIndex = std::enum_value(type);
	const GLenum glType = _priv::BufferTypes[typeIndex];
	void *data = glMapBuffer(glType, glMode);
	checkError();
	unbindBuffer(type);
	return data;
}

void unmapBuffer(Id handle, BufferType type) {
	const int typeIndex = std::enum_value(type);
	const GLenum glType = _priv::BufferTypes[typeIndex];
	if (hasFeature(Feature::DirectStateAccess)) {
		glUnmapNamedBuffer(handle);
	} else {
		bindBuffer(type, handle);
		glUnmapBuffer(glType);
	}
	checkError();
}

bool bindBuffer(BufferType type, Id handle) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = handle;
	core_assert(handle != InvalidId);
	glBindBuffer(glType, handle);
	checkError();
	return true;
}

bool unbindBuffer(BufferType type) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == InvalidId) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = InvalidId;
	glBindBuffer(glType, InvalidId);
	checkError();
	return true;
}

bool bindBufferBase(BufferType type, Id handle, uint32_t index) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::BufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = handle;
	glBindBufferBase(glType, (GLuint)index, handle);
	checkError();
	return true;
}

void genBuffers(uint8_t amount, Id* ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	if (hasFeature(Feature::DirectStateAccess)) {
		glCreateBuffers((GLsizei)amount, (GLuint*)ids);
		checkError();
	} else {
		glGenBuffers((GLsizei)amount, (GLuint*)ids);
		checkError();
	}
}

void deleteBuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glDeleteBuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

IdPtr genSync() {
	return (IdPtr)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void deleteSync(IdPtr& id) {
	if (id == InvalidIdPtr) {
		return;
	}
	static_assert(sizeof(IdPtr) >= sizeof(GLsync), "Unexpected sizes");
	glDeleteSync((GLsync)id);
	id = InvalidId;
}

bool waitForSync(IdPtr id, uint64_t timeout) {
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
	// nano seconds
	const GLenum val = glClientWaitSync((GLsync)id, GL_SYNC_FLUSH_COMMANDS_BIT, (GLuint64)timeout);
	checkError();
	if (val == GL_ALREADY_SIGNALED || val == GL_CONDITION_SATISFIED) {
		return true;
	}
	if (val == GL_TIMEOUT_EXPIRED) {
		return false;
	}
	return false;
}

void genVertexArrays(uint8_t amount, Id* ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glGenVertexArrays((GLsizei)amount, (GLuint*)ids);
	checkError();
}

void deleteShader(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteShader((GLuint)id);
	checkError();
	id = InvalidId;
}

Id genShader(ShaderType type) {
	if (glCreateShader == nullptr) {
		return InvalidId;
	}
	const GLenum glType = _priv::ShaderTypes[std::enum_value(type)];
	const Id id = (Id)glCreateShader(glType);
	checkError();
	return id;
}

void deleteProgram(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteProgram((GLuint)id);
	checkError();
	id = InvalidId;
}

Id genProgram() {
	checkError();
	Id id = (Id)glCreateProgram();
	checkError();
	return id;
}

void deleteVertexArrays(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	for (int i = 0; i < amount; ++i) {
		if (_priv::s.vertexArrayHandle == ids[i]) {
			bindVertexArray(InvalidId);
			break;
		}
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glDeleteVertexArrays((GLsizei)amount, (GLuint*)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteVertexArray(Id& id) {
	if (id == InvalidId) {
		return;
	}
	if (_priv::s.vertexArrayHandle == id) {
		bindVertexArray(InvalidId);
	}
	deleteVertexArrays(1, &id);
	id = InvalidId;
}

void genTextures(uint8_t amount, Id* ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glGenTextures((GLsizei)amount, (GLuint*)ids);
	checkError();
}

void deleteTextures(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glDeleteTextures((GLsizei)amount, (GLuint*)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

bool readFramebuffer(int x, int y, int w, int h, TextureFormat format, uint8_t** pixels) {
	core_assert(_priv::s.framebufferHandle != InvalidId);
	if (_priv::s.framebufferHandle == InvalidId) {
		return false;
	}
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const int pitch = w * f.bits / 8;
	*pixels = (uint8_t*)SDL_malloc(h * pitch);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, w, h, f.dataFormat, f.dataType, (void*)*pixels);
	if (checkError()) {
		SDL_free(*pixels);
		*pixels = nullptr;
		return false;
	}
	return true;
}

void genFramebuffers(uint8_t amount, Id* ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glGenFramebuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id currentFramebuffer() {
	return _priv::s.framebufferHandle;
}

void deleteFramebuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glDeleteFramebuffers((GLsizei)amount, (const GLuint*)ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		if (ids[i] == _priv::s.framebufferHandle) {
			_priv::s.framebufferHandle = InvalidId;
		}
		ids[i] = InvalidId;
	}
}

void genRenderbuffers(uint8_t amount, Id* ids) {
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glGenRenderbuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

void deleteRenderbuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	static_assert(sizeof(Id) == sizeof(GLuint), "Unexpected sizes");
	glDeleteRenderbuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void configureAttribute(const Attribute& a) {
	core_assert(_priv::s.programHandle != InvalidId);
	glEnableVertexAttribArray(a.location);
	checkError();
	const GLenum glType = _priv::DataTypes[std::enum_value(a.type)];
	if (a.typeIsInt) {
		glVertexAttribIPointer(a.location, a.size, glType, a.stride, GL_OFFSET_CAST(a.offset));
		checkError();
	} else {
		glVertexAttribPointer(a.location, a.size, glType, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
		checkError();
	}
	if (a.divisor > 0) {
		glVertexAttribDivisor(a.location, a.divisor);
		checkError();
	}
}

Id genOcclusionQuery() {
	GLuint id;
	glGenQueries(1, &id);
	checkError();
	return (Id)id;
}

Id genTransformFeedback() {
	if (!hasFeature(Feature::TransformFeedback)) {
		return InvalidId;
	}
	GLuint id;
	glGenTransformFeedbacks(1, &id);
	checkError();
	return (Id)id;
}

void deleteTransformFeedback(Id& id) {
	if (id == InvalidId) {
		return;
	}
	if (_priv::s.transformFeedback == id) {
		_priv::s.transformFeedback = InvalidId;
	}
	const GLuint lid = (GLuint)id;
	glDeleteTransformFeedbacks(1, &lid);
	id = InvalidId;
	checkError();
}

bool bindTransformFeedback(Id id) {
	if (id == InvalidId) {
		return false;
	}
	if (_priv::s.transformFeedback == id) {
		return true;
	}
	_priv::s.transformFeedback = id;
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, (GLuint)id);
	return true;
}

bool bindTransforFeebackBuffer(int index, Id bufferId) {
	if (!hasFeature(Feature::TransformFeedback)) {
		return false;
	}
	// the buffer must be of type GL_TRANSFORM_FEEDBACK_BUFFER
	if (bufferId == InvalidId) {
		return false;
	}
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index, (GLuint)bufferId);
	return true;
}

bool beginTransformFeedback(Primitive primitive) {
	if (!hasFeature(Feature::TransformFeedback)) {
		return false;
	}
	const GLenum glMode = _priv::Primitives[std::enum_value(primitive)];
	if (glMode == GL_POINTS || glMode ==  GL_LINES || glMode == GL_TRIANGLES) {
		glBeginTransformFeedback(glMode);
		return true;
	}
	return false;
}

void pauseTransformFeedback() {
	if (!hasFeature(Feature::TransformFeedback)) {
		return;
	}
	glPauseTransformFeedback();
}

void resumeTransformFeedback() {
	if (!hasFeature(Feature::TransformFeedback)) {
		return;
	}
	glResumeTransformFeedback();
}

void endTransformFeedback() {
	if (!hasFeature(Feature::TransformFeedback)) {
		return;
	}
	glEndTransformFeedback();
}

void deleteOcclusionQuery(Id& id) {
	if (id == InvalidId) {
		return;
	}
	if (_priv::s.occlusionQuery == id) {
		_priv::s.occlusionQuery = InvalidId;
	}
	const GLuint lid = (GLuint)id;
#if SANITY_CHECKS_GL
	const GLboolean state = glIsQuery(lid);
	core_assert_always(state == GL_TRUE);
#endif
	glDeleteQueries(1, &lid);
	id = InvalidId;
	checkError();
}

bool isOcclusionQuery(Id id) {
	if (id == InvalidId) {
		return false;
	}
	const GLuint lid = (GLuint)id;
	const GLboolean state = glIsQuery(lid);
	checkError();
	return (bool)state;
}

bool beginOcclusionQuery(Id id) {
	if (_priv::s.occlusionQuery == id || id == InvalidId) {
		return false;
	}
	_priv::s.occlusionQuery = id;
	const GLuint lid = (GLuint)id;
#if SANITY_CHECKS_GL
	const GLboolean state = glIsQuery(lid);
	core_assert_always(state == GL_TRUE);
#endif
	glBeginQuery(GL_SAMPLES_PASSED, lid);
	checkError();
	return true;
}

bool endOcclusionQuery(Id id) {
	if (_priv::s.occlusionQuery != id || id == InvalidId) {
		return false;
	}
	glEndQuery(GL_SAMPLES_PASSED);
	_priv::s.occlusionQuery = video::InvalidId;
	checkError();
	return true;
}

void flush() {
	glFlush();
	checkError();
}

void finish() {
	glFinish();
	checkError();
}

// TODO: cache this per id per frame - or just the last queried id?
bool isOcclusionQueryAvailable(Id id) {
	if (id == InvalidId) {
		return false;
	}
	const GLuint lid = (GLuint)id;
#if SANITY_CHECKS_GL
	const GLboolean state = glIsQuery(lid);
	core_assert_always(state == GL_TRUE);
#endif
	GLint available;
	glGetQueryObjectiv(lid, GL_QUERY_RESULT_AVAILABLE, &available);
	checkError();
	return available != 0;
}

int getOcclusionQueryResult(Id id, bool wait) {
	if (id == InvalidId) {
		return -1;
	}
	const GLuint lid = (GLuint)id;
	if (wait) {
		while (!isOcclusionQueryAvailable(id)) {
		}
		GLint samples;
		glGetQueryObjectiv(lid, GL_QUERY_RESULT, &samples);
		checkError();
		return (int)samples;
	}
	if (!isOcclusionQueryAvailable(id)) {
		return -1;
	}
	GLint samples;
	glGetQueryObjectiv(lid, GL_QUERY_RESULT, &samples);
	checkError();
	return (int)samples;
}

Id bindFramebuffer(Id handle, FrameBufferMode mode) {
	const Id old = _priv::s.framebufferHandle;
#if SANITY_CHECKS_GL
	GLint _oldFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	core_assert_always(_oldFramebuffer == (GLint)old);
#endif
	if (old == handle) {
		return handle;
	}
	_priv::s.framebufferHandle = handle;
	const int typeIndex = std::enum_value(mode);
	const GLenum glType = _priv::FrameBufferModes[typeIndex];
	glBindFramebuffer(glType, handle);
	checkError();
	return old;
}

bool setupRenderBuffer(TextureFormat format, int w, int h, int samples) {
	if (samples > 1) {
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)samples, _priv::TextureFormats[std::enum_value(format)], w, h);
		checkError();
	} else {
		glRenderbufferStorage(GL_RENDERBUFFER, _priv::TextureFormats[std::enum_value(format)], w, h);
		checkError();
	}
	return true;
}

Id bindRenderbuffer(Id handle) {
	if (_priv::s.renderBufferHandle == handle) {
		return handle;
	}
	const Id prev = _priv::s.renderBufferHandle;
	const GLuint lid = (GLuint)handle;
	_priv::s.renderBufferHandle = handle;
	glBindRenderbuffer(GL_RENDERBUFFER, lid);
	checkError();
	return prev;
}

void bufferData(Id handle, BufferType type, BufferMode mode, const void* data, size_t size) {
	if (size <= 0) {
		return;
	}
	const GLuint lid = (GLuint)handle;
	const GLenum usage = _priv::BufferModes[std::enum_value(mode)];
	if (hasFeature(Feature::DirectStateAccess)) {
		glNamedBufferData(lid, (GLsizeiptr)size, data, usage);
		checkError();
	} else {
#if DIRECT_STATE_ACCESS
		void *target = mapBuffer(handle, type, AccessMode::Write);
		memcpy(target, data, size);
		unmapBuffer(handle, type);
#else
		const GLenum glType = _priv::BufferTypes[std::enum_value(type)];
		const Id oldBuffer = boundBuffer(type);
		const bool changed = bindBuffer(type, handle);
		glBufferData(glType, (GLsizeiptr)size, data, usage);
		checkError();
		if (changed) {
			if (oldBuffer == InvalidId) {
				unbindBuffer(type);
			} else {
				bindBuffer(type, oldBuffer);
			}
		}
#endif
	}
	if (_priv::s.vendor[std::enum_value(Vendor::Nouveau)]) {
		// nouveau needs this if doing the buffer update short before the draw call
		glFlush(); // TODO: use glFenceSync here glClientWaitSync
	}
	checkError();
}

size_t bufferSize(BufferType type) {
	const GLenum glType = _priv::BufferTypes[std::enum_value(type)];
	int size;
	glGetBufferParameteriv(glType, GL_BUFFER_SIZE, &size);
	checkError();
	return size;
}

void bufferSubData(Id handle, BufferType type, intptr_t offset, const void* data, size_t size) {
	if (size == 0) {
		return;
	}
	const int typeIndex = std::enum_value(type);
	if (hasFeature(Feature::DirectStateAccess)) {
		const GLuint lid = (GLuint)handle;
		glNamedBufferSubData(lid, (GLintptr)offset, (GLsizeiptr)size, data);
		checkError();
	} else {
#if 1
		const GLenum glType = _priv::BufferTypes[typeIndex];
		const Id oldBuffer = boundBuffer(type);
		const bool changed = bindBuffer(type, handle);
		glBufferSubData(glType, (GLintptr)offset, (GLsizeiptr)size, data);
		checkError();
		if (changed) {
			if (oldBuffer == InvalidId) {
				unbindBuffer(type);
			} else {
				bindBuffer(type, oldBuffer);
			}
		}
#else
		void *target = mapBufferRange(...);
		memcpy(target, data, size);
		unmapBuffer(type);
#endif
	}
}

//TODO: use FrameBufferConfig
void setupDepthCompareTexture(TextureType type, CompareFunc func, TextureCompareMode mode) {
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	const GLenum glMode = _priv::TextureCompareModes[std::enum_value(mode)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, glMode);
	if (mode == TextureCompareMode::RefToTexture) {
		const GLenum glFunc = _priv::CompareFuncs[std::enum_value(func)];
		glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, glFunc);
	}
	checkError();
}

// the fbo is flipped in memory, we have to deal with it here
const glm::vec4& framebufferUV() {
	static const glm::vec4 uv(0.0f, 1.0f, 1.0, 0.0f);
	return uv;
}

bool setupFramebuffer(const std::map<FrameBufferAttachment, TexturePtr>& colorTextures, const std::map<FrameBufferAttachment, RenderBufferPtr>& bufferAttachments) {
	std::vector<GLenum> attachments;
	attachments.reserve(colorTextures.size() + bufferAttachments.size());

	for (auto &bufferAttachment : bufferAttachments) {
		const GLenum glAttachmentType = _priv::FrameBufferAttachments[std::enum_value(bufferAttachment.first)];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttachmentType, GL_RENDERBUFFER, bufferAttachment.second->handle());
		checkError();
		if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
			attachments.push_back(glAttachmentType);
		}
	}

	for (auto &textureAttachment : colorTextures) {
		const TextureType textureTarget = textureAttachment.second->type();
		const GLenum glAttachmentType = _priv::FrameBufferAttachments[std::enum_value(textureAttachment.first)];
		if (textureTarget == TextureType::TextureCube) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachmentType, GL_TEXTURE_CUBE_MAP_POSITIVE_X, textureAttachment.second->handle(), 0);
			checkError();
		} else {
			glFramebufferTexture(GL_FRAMEBUFFER, glAttachmentType, textureAttachment.second->handle(), 0);
			checkError();
		}
		if (glAttachmentType >= GL_COLOR_ATTACHMENT0 && glAttachmentType <= GL_COLOR_ATTACHMENT15) {
			attachments.push_back(glAttachmentType);
		}
	}
	if (attachments.empty()) {
		GLenum buffers[] = {GL_NONE};
		glDrawBuffers(lengthof(buffers), buffers);
		checkError();
	} else {
		if (!checkLimit(attachments.size(), Limit::MaxDrawBuffers)) {
			Log::warn("Max draw buffers exceeded");
			return false;
		}
		std::sort(attachments.begin(), attachments.end());
		glDrawBuffers((GLsizei) attachments.size(), attachments.data());
		checkError();
	}
	return _priv::checkFramebufferStatus();
}

bool bindFrameBufferAttachment(Id texture, FrameBufferAttachment attachment, int layerIndex, bool shouldClear) {
	const GLenum glAttachment = _priv::FrameBufferAttachments[std::enum_value(attachment)];
	glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttachment, (GLuint)texture, 0, layerIndex);
	checkError();
	if (shouldClear) {
		if (attachment == FrameBufferAttachment::Depth) {
			clear(ClearFlag::Depth);
		} else if (attachment == FrameBufferAttachment::Stencil) {
			clear(ClearFlag::Stencil);
		} else if (attachment == FrameBufferAttachment::DepthStencil) {
			clear(ClearFlag::Depth | ClearFlag::Stencil);
		}
	}
	if (!_priv::checkFramebufferStatus()) {
		return false;
	}
	return true;
}

void setupTexture(const TextureConfig& config) {
	const GLenum glType = _priv::TextureTypes[std::enum_value(config.type())];
	if (config.filterMag() != TextureFilter::Max) {
		const GLenum glFilterMag = _priv::TextureFilters[std::enum_value(config.filterMag())];
		glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, glFilterMag);
		checkError();
	}
	if (config.filterMin() != TextureFilter::Max) {
		const GLenum glFilterMin = _priv::TextureFilters[std::enum_value(config.filterMin())];
		glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, glFilterMin);
		checkError();
	}
	if (config.type() == TextureType::Texture3D && config.wrapR() != TextureWrap::Max) {
		const GLenum glWrapR = _priv::TextureWraps[std::enum_value(config.wrapR())];
		glTexParameteri(glType, GL_TEXTURE_WRAP_R, glWrapR);
		checkError();
	}
	if ((config.type() == TextureType::Texture2D || config.type() == TextureType::Texture3D) &&  config.wrapS() != TextureWrap::Max) {
		const GLenum glWrapS = _priv::TextureWraps[std::enum_value(config.wrapS())];
		glTexParameteri(glType, GL_TEXTURE_WRAP_S, glWrapS);
		checkError();
	}
	if (config.wrapT() != TextureWrap::Max) {
		const GLenum glWrapT = _priv::TextureWraps[std::enum_value(config.wrapT())];
		glTexParameteri(glType, GL_TEXTURE_WRAP_T, glWrapT);
		checkError();
	}
	if (config.compareMode() != TextureCompareMode::Max) {
		const GLenum glMode = _priv::TextureCompareModes[std::enum_value(config.compareMode())];
		glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, glMode);
		checkError();
	}
	if (config.compareFunc() != CompareFunc::Max) {
		const GLenum glFunc = _priv::CompareFuncs[std::enum_value(config.compareFunc())];
		glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, glFunc);
		checkError();
	}
	const uint8_t alignment = config.alignment();
	if (alignment > 0u) {
		core_assert(alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8);
		glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	}
	/** Specifies the index of the lowest defined mipmap level. This is an integer value. The initial value is 0. */
	//glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
	/** Sets the index of the highest defined mipmap level. This is an integer value. The initial value is 1000. */
	//glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);
	checkError();
}

void uploadTexture(TextureType type, TextureFormat format, int width, int height, const uint8_t* data, int index) {
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	core_assert(type != TextureType::Max);
	if (type == TextureType::Texture1D) {
		core_assert(height == 1);
		glTexImage1D(glType, 0, f.internalFormat, width, 0, f.dataFormat, f.dataType, (const void*)data);
	} else if (type == TextureType::Texture2D) {
		glTexImage2D(glType, 0, f.internalFormat, width, height, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	} else {
		glTexImage3D(glType, 0, f.internalFormat, width, height, index, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	}
}

void drawElements(Primitive mode, size_t numIndices, DataType type, void* offset) {
	if (numIndices <= 0) {
		return;
	}
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	const GLenum glType = _priv::DataTypes[std::enum_value(type)];
	glDrawElements(glMode, (GLsizei)numIndices, glType, offset);
	checkError();
}

void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount) {
	if (numIndices <= 0) {
		return;
	}
	if (amount <= 0) {
		return;
	}
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	const GLenum glType = _priv::DataTypes[std::enum_value(type)];
	glDrawElementsInstanced(glMode, (GLsizei)numIndices, glType, nullptr, (GLsizei)amount);
	checkError();
}

void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex) {
	if (numIndices <= 0) {
		return;
	}
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	const GLenum glType = _priv::DataTypes[std::enum_value(type)];
	glDrawElementsBaseVertex(glMode, (GLsizei)numIndices, glType, GL_OFFSET_CAST(indexSize * baseIndex), (GLint)baseVertex);
	checkError();
}

void drawArrays(Primitive mode, size_t count) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	glDrawArrays(glMode, (GLint)0, (GLsizei)count);
	checkError();
}

void enableDebug(DebugSeverity severity) {
	if (!hasFeature(Feature::DebugOutput)) {
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

	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
	enable(State::DebugOutput);
	glDebugMessageCallbackARB(_priv::debugOutputCallback, nullptr);
	checkError();
	Log::info("enable opengl debug messages");
}

bool compileShader(Id id, ShaderType shaderType, const std::string& source, const std::string& name) {
	if (id == InvalidId) {
		return false;
	}
	const char *src = source.c_str();
	video::checkError();
	const GLuint lid = (GLuint)id;
	glShaderSource(lid, 1, (const GLchar**) &src, nullptr);
	video::checkError();
	glCompileShader(lid);
	video::checkError();

	GLint status;
	glGetShaderiv(lid, GL_COMPILE_STATUS, &status);
	video::checkError();
	GLint infoLogLength = 0;
	glGetShaderiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);
	video::checkError();

	if (infoLogLength > 0) {
		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(lid, infoLogLength, nullptr, strInfoLog.get());
		video::checkError();
		const std::string compileLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));

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
			Log::error("Shader source:\n%s", source.c_str());
		} else {
			Log::info("%s: %s", name.c_str(), compileLog.c_str());
		}
	}
	if (status != GL_TRUE) {
		deleteShader(id);
		return false;
	}
	return true;
}

bool bindTransformFeedbackVaryings(Id program, TransformFeedbackCaptureMode mode, const std::vector<std::string>& varyings) {
	if (!hasFeature(Feature::TransformFeedback)) {
		return false;
	}
	if (varyings.empty() || mode == TransformFeedbackCaptureMode::Max) {
		// nothing to do is success
		return true;
	}
	std::vector<const GLchar*> transformFeedbackStarts(varyings.size());
	for (auto& transformFeedbackVaryings : varyings) {
		transformFeedbackStarts.push_back(transformFeedbackVaryings.c_str());
	}
	glTransformFeedbackVaryings((GLuint) program,
			(GLsizei) transformFeedbackStarts.size(),
			transformFeedbackStarts.data(),
			(GLenum)_priv::TransformFeedbackCaptureModes[std::enum_value(mode)]);
	return true;
}

bool linkComputeShader(Id program, Id comp, const std::string& name) {
	const GLuint lid = (GLuint)program;
	glAttachShader(lid, comp);
	glLinkProgram(lid);
	GLint status;
	glGetProgramiv(lid, GL_LINK_STATUS, &status);
	checkError();
	GLint infoLogLength;
	glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 0) {
		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(lid, infoLogLength, nullptr, strInfoLog.get());
		video::checkError();
		const std::string linkLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));
		if (status != GL_TRUE) {
			Log::error("Failed to link: %s\n%s", name.c_str(), linkLog.c_str());
		} else {
			Log::info("%s: %s", name.c_str(), linkLog.c_str());
		}
	}
	glDetachShader(lid, comp);
	checkError();
	if (status != GL_TRUE) {
		deleteProgram(program);
		return false;
	}

#ifdef DEBUG
	glValidateProgram(lid);
	GLint success, logLength;
	glGetProgramiv(lid, GL_VALIDATE_STATUS, &success);
	if (success == GL_FALSE) {
		Log::error("Failed to validate: %s", name.c_str());
	}
	glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		std::string message(logLength, '\n');
		if (message.size() > 1) {
			glGetProgramInfoLog(lid, message.size(), nullptr, &message[0]);
		}
		message.resize(std::max(logLength, 1) - 1);
		Log::info("Validation output: %s\n%s", name.c_str(), message.c_str());
	}
#endif
	return true;
}

bool bindImage(Id textureHandle, AccessMode mode, ImageFormat format) {
	if (_priv::s.imageHandle == textureHandle && _priv::s.imageFormat == format && _priv::s.imageAccessMode == mode) {
		return false;
	}
	core_assert(glBindImageTexture != nullptr);
	const GLenum glFormat = _priv::ImageFormatTypes[std::enum_value(format)];
	const GLenum glAccessMode = _priv::AccessModes[std::enum_value(mode)];
	const GLuint unit = 0u;
	const GLint level = 0;
	const GLboolean layered = GL_FALSE;
	const GLint layer = 0;
	glBindImageTexture(unit, (GLuint)textureHandle, level, layered, layer, glAccessMode, glFormat);
	return true;
}

bool runShader(Id program, const glm::uvec3& workGroups, bool wait) {
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
	glDispatchCompute((GLuint)workGroups.x, (GLuint)workGroups.y, (GLuint)workGroups.z);
	if (wait && glMemoryBarrier != nullptr) {
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	return false;
}

bool linkShader(Id program, Id vert, Id frag, Id geom, const std::string& name) {
	const GLuint lid = (GLuint)program;
	glAttachShader(lid, (GLuint)vert);
	glAttachShader(lid, (GLuint)frag);
	if (geom != InvalidId) {
		glAttachShader(lid, (GLuint)geom);
	}

	glLinkProgram(lid);
	GLint status;
	glGetProgramiv(lid, GL_LINK_STATUS, &status);
	checkError();
	GLint infoLogLength;
	glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 0) {
		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(lid, infoLogLength, nullptr, strInfoLog.get());
		video::checkError();
		const std::string linkLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));
		if (status != GL_TRUE) {
			Log::error("Failed to link: %s\n%s", name.c_str(), linkLog.c_str());
		} else {
			Log::info("%s: %s", name.c_str(), linkLog.c_str());
		}
	}
	glDetachShader(lid, (GLuint)vert);
	glDetachShader(lid, (GLuint)frag);
	if (geom != InvalidId) {
		glDetachShader(lid, (GLuint)geom);
	}
	checkError();
	if (status != GL_TRUE) {
		deleteProgram(program);
		return false;
	}

#if 0
	glValidateProgram(lid);
	GLint success, logLength;
	glGetProgramiv(lid, GL_VALIDATE_STATUS, &success);
	if (success == GL_FALSE) {
		Log::error("Failed to validate: %s", name.c_str());
	}
	glGetProgramiv(lid, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		std::string message(logLength, '\n');
		if (message.size() > 1) {
			glGetProgramInfoLog(lid, message.size(), nullptr, &message[0]);
		}
		message.resize(std::max(logLength, 1) - 1);
		Log::info("Validation output: %s\n%s", name.c_str(), message.c_str());
	}
#endif
	return true;
}

int fetchUniforms(Id program, ShaderUniforms& uniforms, const std::string& name) {
	int n = _priv::fillUniforms(program, uniforms, name, GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, glGetActiveUniformName, glGetUniformLocation, false);
	n += _priv::fillUniforms(program, uniforms, name, GL_ACTIVE_UNIFORM_BLOCKS, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, glGetActiveUniformBlockName, glGetUniformBlockIndex, true);
	return n;
}

int fetchAttributes(Id program, ShaderAttributes& attributes, const std::string& name) {
	char varName[MAX_SHADER_VAR_NAME];
	int numAttributes = 0;
	const GLuint lid = (GLuint)program;
	glGetProgramiv(lid, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	checkError();

	for (int i = 0; i < numAttributes; ++i) {
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveAttrib(lid, i, MAX_SHADER_VAR_NAME - 1, &length, &size, &type, varName);
		const int location = glGetAttribLocation(lid, varName);
		attributes[varName] = location;
		Log::debug("attribute location for %s is %i (shader %s)", varName, location, name.c_str());
	}
	return numAttributes;
}

void destroyContext(RendererContext& context) {
	SDL_GL_DeleteContext((SDL_GLContext)context);
}

RendererContext createContext(SDL_Window* window) {
	core_assert(window != nullptr);
	return SDL_GL_CreateContext(window);
}

void startFrame(SDL_Window* window, RendererContext& context) {
	SDL_GL_MakeCurrent(window, (SDL_GLContext)context);
}

void endFrame(SDL_Window* window) {
	SDL_GL_SwapWindow(window);
}

void setup() {
	const core::VarPtr& glVersion = core::Var::getSafe(cfg::ClientOpenGLVersion);
	int glMinor = 0, glMajor = 0;
	if (std::sscanf(glVersion->strVal().c_str(), "%3i.%3i", &glMajor, &glMinor) != 2) {
		const GLVersion& version = GL4_3;
		glMajor = version.majorVersion;
		glMinor = version.minorVersion;
	}
	Log::info("Request gl context %i.%i", glMajor, glMinor);
	GLVersion glv(glMajor, glMinor);
	for (size_t i = 0; i < SDL_arraysize(GLVersions); ++i) {
		if (GLVersions[i].version == glv) {
			Shader::glslVersion = GLVersions[i].glslVersion;
		}
	}

	SDL_ClearError();
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	const core::VarPtr& multisampleBuffers = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	const core::VarPtr& multisampleSamples = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	int samples = multisampleSamples->intVal();
	int buffers = multisampleBuffers->intVal();
	if (samples <= 0) {
		buffers = 0;
	} else if (buffers <= 0) {
		samples = 0;
	}
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glv.majorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glv.minorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
}

void resize(int windowWidth, int windowHeight, float scaleFactor) {
	_priv::s.windowWidth = windowWidth;
	_priv::s.windowHeight = windowHeight;
	_priv::s.scaleFactor = scaleFactor;
}

glm::ivec2 getWindowSize() {
	return glm::ivec2(_priv::s.windowWidth, _priv::s.windowHeight);
}

float getScaleFactor() {
	return _priv::s.scaleFactor;
}

bool init(int windowWidth, int windowHeight, float scaleFactor) {
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &_priv::s.glVersion.majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &_priv::s.glVersion.minorVersion);
	Log::info("got gl context: %i.%i", _priv::s.glVersion.majorVersion, _priv::s.glVersion.minorVersion);

	resize(windowWidth, windowHeight, scaleFactor);

	if (flextInit() == -1) {
		Log::error("Could not initialize opengl: %s", SDL_GetError());
		return false;
	}

	_priv::setupLimitsAndSpecs();
	_priv::setupFeatures();

	const char *glvendor = (const char*)glGetString(GL_VENDOR);
	const char* glrenderer = (const char*)glGetString(GL_RENDERER);
	const char* glversion = (const char*)glGetString(GL_VERSION);
	Log::info("GL_VENDOR: %s", glvendor);
	Log::info("GL_RENDERER: %s", glrenderer);
	Log::info("GL_VERSION: %s", glversion);
	if (glvendor != nullptr) {
		const std::string vendor(glvendor);
		for (int i = 0; i < std::enum_value(Vendor::Max); ++i) {
			const bool match = core::string::icontains(vendor, _priv::VendorStrings[i]);
			_priv::s.vendor[i] = match;
		}
	}

	for (int i = 0; i < std::enum_value(Vendor::Max); ++i) {
		if (_priv::s.vendor[i]) {
			Log::info("Found vendor: %s", _priv::VendorStrings[i]);
		} else {
			Log::debug("Didn't find vendor: %s", _priv::VendorStrings[i]);
		}
	}

	const bool vsync = core::Var::getSafe(cfg::ClientVSync)->boolVal();
	if (vsync) {
		if (SDL_GL_SetSwapInterval(-1) == -1) {
			if (SDL_GL_SetSwapInterval(1) == -1) {
				Log::warn("Could not activate vsync: %s", SDL_GetError());
			}
		}
	} else {
		SDL_GL_SetSwapInterval(0);
	}
	if (SDL_GL_GetSwapInterval() == 0) {
		Log::info("Deactivated vsync");
	} else {
		Log::info("Activated vsync");
	}

	if (hasFeature(Feature::DirectStateAccess)) {
		Log::info("Use direct state access");
	}

	const core::VarPtr& multisampleBuffers = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	const core::VarPtr& multisampleSamples = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	bool multisampling = multisampleSamples->intVal() > 0 && multisampleBuffers->intVal() > 0;
	int buffers, samples;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
	if (buffers == 0 || samples == 0) {
		Log::warn("Could not get FSAA context");
		multisampling = false;
	} else {
		Log::info("Got FSAA context with %i buffers and %i samples", buffers, samples);
	}

	int profile;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);
	if (profile == SDL_GL_CONTEXT_PROFILE_CORE) {
		Log::info("Got core profile");
	} else if (profile == SDL_GL_CONTEXT_PROFILE_ES) {
		Log::info("Got ES profile");
	} else if (profile == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
		Log::info("Got compatibility profile");
	} else {
		Log::warn("Unknown profile: %i", profile);
	}

	if (multisampling) {
		video::enable(video::State::MultiSample);
	}
	return true;
}

RenderState& renderState() {
	return s;
}

}
