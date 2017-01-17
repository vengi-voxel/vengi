#include "video/Renderer.h"
#include "GLTypes.h"
#include "GLVersion.h"
#include "GLFunc.h"
#include "GLState.h"
#include "GLMapping.h"
#include "GLHelper.h"
#include "core/Common.h"
#include "core/Log.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <SDL.h>

namespace video {

#define SANITY_CHECKS_GL 0

void checkError() {
#ifdef DEBUG
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

		core_assert_msg(glError == GL_NO_ERROR, "GL err: %s => %i", error, glError);
	}
#endif
}

bool bindDepthTexture(int textureIndex, DepthBufferMode mode, Id depthTexture) {
	const bool depthCompare = mode == DepthBufferMode::DEPTH_CMP;
	const bool depthAttachment = mode == DepthBufferMode::DEPTH || depthCompare;
	if (depthAttachment) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, textureIndex);
		clear(ClearFlag::Depth);
	} else {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, depthTexture, 0, textureIndex);
		clear(ClearFlag::Color | ClearFlag::Depth);
	}

	if (!_priv::checkFramebufferStatus()) {
		return false;
	}
	checkError();
	return true;
}

void readBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	checkError();
}

bool setupDepthbuffer(Id fbo, DepthBufferMode mode) {
	const Id prev = bindFramebuffer(FrameBufferMode::Default, fbo);
	const bool depthCompare = mode == DepthBufferMode::DEPTH_CMP;
	const bool depthAttachment = mode == DepthBufferMode::DEPTH || depthCompare;

	if (depthAttachment) {
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	} else {
		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	}
	checkError();
	bindFramebuffer(FrameBufferMode::Default, prev);
	return true;
}

bool setupGBuffer(Id fbo, const glm::ivec2& dimension, Id* textures, size_t texCount, Id depthTexture) {
	const Id prev = bindFramebuffer(FrameBufferMode::Default, fbo);

	for (std::size_t i = 0; i < texCount; ++i) {
		bindTexture(TextureUnit::Upload, TextureType::Texture2D, textures[i]);
		// we are going to write vec3 into the out vars in the shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, dimension.x, dimension.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	bindTexture(TextureUnit::Upload, TextureType::Texture2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);

	const bool retVal = _priv::checkFramebufferStatus();
	bindFramebuffer(FrameBufferMode::Default, prev);
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

	for (unsigned int i = 1; i <= 6; i++) {
		const image::ImagePtr& img = images[i];
		const GLenum mode = img->depth() == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(types[i - 1], 0, mode, img->width(), img->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data());
	}

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
	if (_priv::s.lineAntialiasing) {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.smoothedLineWidth.x, _priv::s.smoothedLineWidth.y));
	} else {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.aliasedLineWidth.x, _priv::s.aliasedLineWidth.y));
	}
	checkError();
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
	glClear(glValue);
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
	if (_priv::s.scissorX == x && _priv::s.scissorY == y && _priv::s.scissorW == w && _priv::s.scissorH == h) {
		return false;
	}
	_priv::s.scissorX = x;
	_priv::s.scissorY = y;
	_priv::s.scissorW = w;
	_priv::s.scissorH = h;
	glScissor((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	checkError();
	return true;
}

bool enable(State state) {
	if (state == State::DepthMask) {
		if (_priv::s.depthMask) {
			return true;
		}
		glDepthMask(GL_TRUE);
		checkError();
		_priv::s.depthMask = true;
		return false;
	}

	const int stateIndex = std::enum_value(state);
	if (_priv::s.states[stateIndex]) {
		return true;
	}
	_priv::s.states[stateIndex] = true;
	glEnable(_priv::States[stateIndex]);
	checkError();
	return false;
}

bool disable(State state) {
	if (state == State::DepthMask) {
		if (!_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_FALSE);
		checkError();
		_priv::s.depthMask = false;
		return true;
	}

	const int stateIndex = std::enum_value(state);
	if (!_priv::s.states[stateIndex]) {
		return false;
	}
	_priv::s.states[stateIndex] = false;
	glDisable(_priv::States[stateIndex]);
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

bool polygonMode(Face face, PolygonMode mode) {
	if (_priv::s.polygonModeFace == face && _priv::s.polygonMode == mode) {
		return false;
	}
	_priv::s.polygonModeFace = face;
	_priv::s.polygonMode = mode;
	const GLenum glMode = _priv::PolygonModes[std::enum_value(mode)];
	const GLenum glFace = _priv::Faces[std::enum_value(face)];
	glPolygonMode(glFace, glMode);
	checkError();
	return true;
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

bool activeTextureUnit(TextureUnit unit) {
	if (_priv::s.textureUnit == unit) {
		return false;
	}
	const GLenum glUnit = _priv::TextureUnits[std::enum_value(unit)];
	glActiveTexture(glUnit);
	checkError();
	_priv::s.textureUnit = unit;
	return true;
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	const bool changeUnit = activeTextureUnit(unit);
	if (changeUnit || _priv::s.textureHandle != handle) {
		_priv::s.textureHandle = handle;
		glBindTexture(_priv::TextureTypes[std::enum_value(type)], handle);
		checkError();
		return true;
	}
	return false;
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

bool bindVertexArray(Id handle) {
	if (_priv::s.vertexArrayHandle == handle) {
		return false;
	}
	glBindVertexArray(handle);
	checkError();
	_priv::s.vertexArrayHandle = handle;
	return true;
}

bool bindBuffer(VertexBufferType type, Id handle) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::VertexBufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = handle;
	core_assert(handle != InvalidId);
	glBindBuffer(glType, handle);
	checkError();
	return true;
}

bool unbindBuffer(VertexBufferType type) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == InvalidId) {
		return false;
	}
	const GLenum glType = _priv::VertexBufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = InvalidId;
	glBindBuffer(glType, InvalidId);
	checkError();
	return true;
}

bool bindBufferBase(VertexBufferType type, Id handle, uint32_t index) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::VertexBufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = handle;
	glBindBufferBase(glType, (GLuint)index, handle);
	checkError();
	return true;
}

void genBuffers(uint8_t amount, Id* ids) {
	glGenBuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genBuffer() {
	Id id;
	genBuffers(1, &id);
	return id;
}

void deleteBuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteBuffers((GLsizei)amount, ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteBuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteBuffers(1, &id);
	id = InvalidId;
}

void genVertexArrays(uint8_t amount, Id* ids) {
	glGenVertexArrays((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genVertexArray() {
	Id id;
	genVertexArrays(1, &id);
	return id;
}

void deleteShader(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteShader(id);
	checkError();
	id = InvalidId;
}

Id genShader(ShaderType type) {
	const GLenum glType = _priv::ShaderTypes[std::enum_value(type)];
	const Id id = glCreateShader(glType);
	checkError();
	return id;
}

void deleteProgram(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteProgram(id);
	checkError();
	id = InvalidId;
}

Id genProgram() {
	Id id = glCreateProgram();
	checkError();
	return id;
}

void deleteVertexArrays(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteVertexArrays((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteVertexArray(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteVertexArrays(1, &id);
	id = InvalidId;
}

void genTextures(uint8_t amount, Id* ids) {
	glGenTextures((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genTexture() {
	Id id;
	genTextures(1, &id);
	return id;
}

void deleteTextures(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteTextures((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteTextures(1, &id);
	id = InvalidId;
}

void genFramebuffers(uint8_t amount, Id* ids) {
	glGenFramebuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genFramebuffer() {
	Id id;
	genFramebuffers(1, &id);
	return id;
}

void deleteFramebuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteFramebuffers((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteFramebuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteFramebuffers(1, &id);
	id = InvalidId;
}

void genRenderbuffers(uint8_t amount, Id* ids) {
	glGenRenderbuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genRenderbuffer() {
	Id id;
	genRenderbuffers(1, &id);
	return id;
}

void deleteRenderbuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteRenderbuffers((GLsizei)amount, ids);
	checkError();
	for (uint8_t i = 0u; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteRenderbuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteRenderbuffers(1, &id);
	id = InvalidId;
}

void configureAttribute(const Attribute& a) {
	glEnableVertexAttribArray(a.index);
	const GLenum glType = _priv::DataTypes[std::enum_value(a.type)];
	if (a.typeIsInt) {
		glVertexAttribIPointer(a.index, a.size, glType, a.stride, GL_OFFSET_CAST(a.offset));
	} else {
		glVertexAttribPointer(a.index, a.size, glType, a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
	}
	if (a.divisor > 0) {
		glVertexAttribDivisor(a.index, a.divisor);
	}
	checkError();
}

Id bindFramebuffer(FrameBufferMode mode, Id handle, Id textureHandle) {
	const Id old = _priv::s.framebufferHandle;
#if SANITY_CHECKS_GL
	GLint _oldFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	core_assert(_oldFramebuffer == (GLint)old);
#endif
	if (old == handle && _priv::s.framebufferTextureHandle == textureHandle) {
		return handle;
	}
	_priv::s.framebufferHandle = handle;
	_priv::s.framebufferTextureHandle = textureHandle;
	const int typeIndex = std::enum_value(mode);
	const GLenum glType = _priv::FrameBufferModes[typeIndex];
	glBindFramebuffer(glType, handle);
	if (textureHandle != InvalidId) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);
	}
	checkError();
	return old;
}

bool bindRenderbuffer(Id handle) {
	glBindRenderbuffer(GL_RENDERBUFFER, handle);
	checkError();
	return true;
}

void bufferData(VertexBufferType type, VertexBufferMode mode, const void* data, size_t size) {
	const GLenum glType = _priv::VertexBufferTypes[std::enum_value(type)];
	const GLenum usage = _priv::VertexBufferModes[std::enum_value(mode)];
	glBufferData(glType, (GLsizeiptr)size, data, usage);
	checkError();
}

void bufferSubData(VertexBufferType type, intptr_t offset, const void* data, size_t size) {
	const GLenum glType = _priv::VertexBufferTypes[std::enum_value(type)];
	glBufferSubData(glType, (GLintptr)offset, (GLsizeiptr)size, data);
	checkError();
}

void disableDepthCompareTexture(TextureUnit unit, TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	checkError();
}

void setupDepthCompareTexture(TextureUnit unit, TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	checkError();
}

bool setupFramebuffer(Id& fbo, Id& texture, Id& depth, const glm::ivec2& dimension) {
	fbo = genFramebuffer();
	Id prev = bindFramebuffer(FrameBufferMode::Default, fbo);
	texture = genTexture();
	bindTexture(TextureUnit::Upload, TextureType::Texture2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimension.x, dimension.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	depth = genRenderbuffer();
	bindRenderbuffer(depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, dimension.x, dimension.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

	checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	const bool retVal = _priv::checkFramebufferStatus();
	bindFramebuffer(FrameBufferMode::Default, prev);
	return retVal;
}

void setupTexture(TextureType type, TextureWrap wrap) {
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	if (wrap != TextureWrap::None) {
		const GLenum glWrap = _priv::TextureWraps[std::enum_value(wrap)];
		glTexParameteri(glType, GL_TEXTURE_WRAP_S, glWrap);
		glTexParameteri(glType, GL_TEXTURE_WRAP_T, glWrap);
	}
	glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);
	checkError();
}

void uploadTexture(TextureType type, TextureFormat format, int width, int height, const uint8_t* data, int index) {
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	if (type == TextureType::Texture2D) {
		glTexImage2D(glType, 0, f.internalFormat, width, height, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	} else {
		glTexImage3D(glType, 0, f.internalFormat, width, height, index, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	}
}

void drawElements(Primitive mode, size_t numIndices, DataType type) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	const GLenum glType = _priv::DataTypes[std::enum_value(type)];
	glDrawElements(glMode, (GLsizei)numIndices, glType, nullptr);
	checkError();
}

void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	const GLenum glType = _priv::DataTypes[std::enum_value(type)];
	glDrawElementsInstanced(glMode, (GLsizei)numIndices, glType, nullptr, (GLsizei)amount);
	checkError();
}

void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex) {
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

void disableDebug() {
	if (!hasFeature(Feature::DebugOutput)) {
		return;
	}
	disable(State::DebugOutput);
	Log::info("disable opengl debug messages");
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
	if (glDebugMessageControlARB != nullptr) {
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
		enable(State::DebugOutput);
		glDebugMessageCallbackARB(_priv::debugOutputCallback, nullptr);
		checkError();
		Log::info("enable opengl debug messages");
	}
}

bool compileShader(Id id, ShaderType shaderType, const std::string& source) {
	const char *s = source.c_str();
	glShaderSource(id, 1, (const GLchar**) &s, nullptr);
	glCompileShader(id);

	GLint status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (!status) {
		GLint infoLogLength = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(id, infoLogLength, nullptr, strInfoLog.get());
		const std::string errorLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));

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
		default:
			strShaderType = "unknown";
			break;
		}

		Log::error("Failed to compile:\n----- \n%s\n-----\nshaderType: %s\nerrorlog: %s", source.c_str(), strShaderType, errorLog.c_str());
		video::deleteShader(id);
		return false;
	}

	return true;
}

void linkShader(Id program, Id vert, Id frag, Id geom) {
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	if (geom != InvalidId) {
		glAttachShader(program, geom);
	}

	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	video::checkError();
	if (status) {
		glDetachShader(program, vert);
		glDetachShader(program, frag);
		if (geom != InvalidId) {
			glDetachShader(program, geom);
		}
		return;
	}
	GLint infoLogLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

	GLchar* strInfoLog = new GLchar[infoLogLength + 1];
	glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);
	strInfoLog[infoLogLength] = '\0';
	Log::error("linker failure: %s", strInfoLog);
	video::deleteProgram(program);
	delete[] strInfoLog;
}

bool hasFeature(Feature f) {
	return _priv::s.features[std::enum_value(f)];
}

bool init() {
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &_priv::s.glVersion.majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &_priv::s.glVersion.minorVersion);
	Log::info("got gl context: %i.%i", _priv::s.glVersion.majorVersion, _priv::s.glVersion.minorVersion);

	GLLoadFunctions();

	_priv::setupLimits();
	_priv::setupFeatures();
	return true;
}

}
