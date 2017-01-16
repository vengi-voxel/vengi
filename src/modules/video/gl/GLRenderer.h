#pragma once

#include "GLTypes.h"
#include "core/Common.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

inline const char* translateError(GLenum glError) {
#define GL_ERROR_TRANSLATE(e) case e: return #e;
	switch (glError) {
	/* openGL errors */
	GL_ERROR_TRANSLATE(GL_INVALID_ENUM)
	GL_ERROR_TRANSLATE(GL_INVALID_VALUE)
	GL_ERROR_TRANSLATE(GL_INVALID_OPERATION)
	GL_ERROR_TRANSLATE(GL_OUT_OF_MEMORY)
	default:
		return "UNKNOWN";
	}
#undef GL_ERROR_TRANSLATE
}

inline void checkError() {
#ifdef DEBUG
	/* check gl errors (can return multiple errors) */
	for (;;) {
		const GLenum glError = glGetError();
		if (glError == GL_NO_ERROR) {
			break;
		}

		core_assert_msg(glError == GL_NO_ERROR, "GL err: %s => %i", translateError(glError), glError);
	}
#endif
}

namespace _priv {
	struct GLState {
		glm::vec4 clearColor;
		bool depthMask = false;
		Face cullFace = Face::Max;
		CompareFunc depthFunc = CompareFunc::Max;
		Id programHandle = InvalidId;
		Id vertexArrayHandle = InvalidId;
		glm::vec2 polygonOffset;
		Face polygonModeFace = Face::Max;
		PolygonMode polygonMode = PolygonMode::Max;
		BlendMode blendSrc = BlendMode::Max;
		BlendMode blendDest = BlendMode::Max;
		TextureUnit textureUnit = TextureUnit::Max;
		Id textureHandle = InvalidId;
		int viewportX = 0;
		int viewportY = 0;
		int viewportW = 0;
		int viewportH = 0;
		int scissorX = 0;
		int scissorY = 0;
		int scissorW = 0;
		int scissorH = 0;
		bool states[std::enum_value(State::Max)] = {};
	};
	static GLState s;

	static const struct Formats {
		uint8_t bits;
		GLenum internalFormat;
		GLenum dataFormat;
		GLenum dataType;
	} textureFormats[] = {
		{32, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
		{24, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},
		{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}
	};

	static GLenum ShaderTypes[] {
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER,
		GL_GEOMETRY_SHADER
	};
	static_assert(std::enum_value(ShaderType::Max) == (int)SDL_arraysize(ShaderTypes), "Array sizes don't match Max");

	static GLenum FrameBufferModes[] {
		GL_READ_FRAMEBUFFER,
		GL_DRAW_FRAMEBUFFER,
		GL_FRAMEBUFFER
	};
	static_assert(std::enum_value(FrameBufferMode::Max) == (int)SDL_arraysize(FrameBufferModes), "Array sizes don't match Max");

	static GLenum VertexBufferModes[] {
		GL_STATIC_DRAW,
		GL_DYNAMIC_DRAW,
		GL_STREAM_DRAW
	};
	static_assert(std::enum_value(VertexBufferMode::Max) == (int)SDL_arraysize(VertexBufferModes), "Array sizes don't match Max");

	static GLenum VertexBufferTypes[] {
		GL_ARRAY_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER,
		GL_UNIFORM_BUFFER
	};
	static_assert(std::enum_value(VertexBufferType::Max) == (int)SDL_arraysize(VertexBufferTypes), "Array sizes don't match Max");

	static GLenum States[] {
		0,
		GL_DEPTH_TEST,
		GL_CULL_FACE,
		GL_BLEND,
		GL_POLYGON_OFFSET_FILL,
		GL_POLYGON_OFFSET_POINT,
		GL_POLYGON_OFFSET_LINE,
		GL_SCISSOR_TEST,
		GL_MULTISAMPLE,
		GL_LINE_SMOOTH
	};
	static_assert(std::enum_value(State::Max) == (int)SDL_arraysize(States), "Array sizes don't match Max");

	static GLenum TextureTypes[] {
		GL_TEXTURE_2D,
		GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_CUBE_MAP
	};
	static_assert(std::enum_value(TextureType::Max) == (int)SDL_arraysize(TextureTypes), "Array sizes don't match Max");

	static GLenum TextureWraps[] {
		GL_CLAMP_TO_EDGE,
		GL_REPEAT
	};
	static_assert(std::enum_value(TextureWrap::Max) == (int)SDL_arraysize(TextureWraps), "Array sizes don't match Max");

	static GLenum BlendModes[] {
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
	static_assert(std::enum_value(BlendMode::Max) == (int)SDL_arraysize(BlendModes), "Array sizes don't match Max");

	static GLenum CompareFuncs[] {
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS
	};
	static_assert(std::enum_value(CompareFunc::Max) == (int)SDL_arraysize(CompareFuncs), "Array sizes don't match Max");
}

inline bool clearColor(const glm::vec4& clearColor) {
	if (_priv::s.clearColor == clearColor) {
		return false;
	}
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	_priv::s.clearColor = clearColor;
	return true;
}

inline void clear(ClearFlag flag) {
	glClear(std::enum_value(flag));
}

inline bool viewport(int x, int y, int w, int h) {
	if (_priv::s.viewportX == x && _priv::s.viewportY == y && _priv::s.viewportW == w && _priv::s.viewportH == h) {
		return false;
	}
	_priv::s.viewportX = x;
	_priv::s.viewportY = y;
	_priv::s.viewportW = w;
	_priv::s.viewportH = h;
	glViewport((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	return true;
}

inline void getViewport(int& x, int& y, int& w, int& h) {
	x = _priv::s.viewportX;
	y = _priv::s.viewportY;
	w = _priv::s.viewportW;
	h = _priv::s.viewportH;
}

inline bool scissor(int x, int y, int w, int h) {
	if (_priv::s.scissorX == x && _priv::s.scissorY == y && _priv::s.scissorW == w && _priv::s.scissorH == h) {
		return false;
	}
	_priv::s.scissorX = x;
	_priv::s.scissorY = y;
	_priv::s.scissorW = w;
	_priv::s.scissorH = h;
	glScissor((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	return true;
}

inline bool enable(State state) {
	if (state == State::DepthMask) {
		if (_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_TRUE);
		_priv::s.depthMask = true;
		return true;
	}

	const int stateIndex = std::enum_value(state);
	if (_priv::s.states[stateIndex]) {
		return false;
	}
	_priv::s.states[stateIndex] = true;
	glEnable(_priv::States[stateIndex]);
	return true;
}

inline bool disable(State state) {
	if (state == State::DepthMask) {
		if (!_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_FALSE);
		_priv::s.depthMask = false;
		return true;
	}

	const int stateIndex = std::enum_value(state);
	if (!_priv::s.states[stateIndex]) {
		return false;
	}
	_priv::s.states[stateIndex] = false;
	glDisable(_priv::States[stateIndex]);
	return true;
}

inline bool cullFace(Face face) {
	if (_priv::s.cullFace == face) {
		return false;
	}
	glCullFace(std::enum_value(face));
	_priv::s.cullFace = face;
	return true;
}

inline bool depthFunc(CompareFunc func) {
	if (_priv::s.depthFunc == func) {
		return false;
	}
	glDepthFunc(_priv::CompareFuncs[std::enum_value(func)]);
	_priv::s.depthFunc = func;
	return true;
}

inline bool blendFunc(BlendMode src, BlendMode dest) {
	if (_priv::s.blendSrc == src && _priv::s.blendDest == dest) {
		return false;
	}
	_priv::s.blendSrc = src;
	_priv::s.blendDest = dest;
	const GLenum glSrc = _priv::BlendModes[std::enum_value(src)];
	const GLenum glDest = _priv::BlendModes[std::enum_value(dest)];
	glBlendFunc(glSrc, glDest);
	return true;
}

inline bool polygonMode(Face face, PolygonMode mode) {
	if (_priv::s.polygonModeFace == face && _priv::s.polygonMode == mode) {
		return false;
	}
	_priv::s.polygonModeFace = face;
	_priv::s.polygonMode = mode;
	glPolygonMode(std::enum_value(face), std::enum_value(mode));
	return true;
}

inline bool polygonOffset(const glm::vec2& offset) {
	if (_priv::s.polygonOffset == offset) {
		return false;
	}
	glPolygonOffset(offset.x, offset.y);
	_priv::s.polygonOffset = offset;
	return true;
}

inline bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	bool changeUnit = false;
	if (_priv::s.textureUnit != unit) {
		glActiveTexture(std::enum_value(unit));
		_priv::s.textureUnit = unit;
		changeUnit = true;
	}
	if (changeUnit || _priv::s.textureHandle != handle) {
		_priv::s.textureHandle = handle;
		glBindTexture(_priv::TextureTypes[std::enum_value(type)], handle);
		return true;
	}
	return false;
}

inline bool useProgram(Id handle) {
	if (_priv::s.programHandle == handle) {
		return false;
	}
	glUseProgram(handle);
	_priv::s.programHandle = handle;
	return true;
}

inline bool bindVertexArray(Id handle) {
	if (_priv::s.vertexArrayHandle == handle) {
		return false;
	}
	glBindVertexArray(handle);
	_priv::s.vertexArrayHandle = handle;
	return true;
}

inline bool bindBuffer(VertexBufferType type, Id handle) {
	glBindBuffer(_priv::VertexBufferTypes[std::enum_value(type)], handle);
	return true;
}

inline void genBuffers(uint8_t amount, Id* ids) {
	glGenBuffers((GLsizei)amount, (GLuint*)ids);
}

inline Id genBuffer() {
	Id id;
	genBuffers(1, &id);
	return id;
}

inline void deleteBuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteBuffers((GLsizei)amount, ids);
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

inline void deleteBuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteBuffers(1, &id);
}

inline void genVertexArrays(uint8_t amount, Id* ids) {
	glGenVertexArrays((GLsizei)amount, (GLuint*)ids);
}

inline Id genVertexArray() {
	Id id;
	genVertexArrays(1, &id);
	return id;
}

inline void deleteShader(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteShader(id);
	id = InvalidId;
}

inline Id genShader(ShaderType type) {
	return glCreateShader(_priv::ShaderTypes[std::enum_value(type)]);
}

inline void deleteProgram(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteProgram(id);
	id = InvalidId;
}

inline Id genProgram() {
	return glCreateProgram();
}

inline void deleteVertexArrays(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteVertexArrays((GLsizei)amount, ids);
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

inline void deleteVertexArray(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteVertexArrays(1, &id);
}

inline void genTextures(uint8_t amount, Id* ids) {
	glGenTextures((GLsizei)amount, (GLuint*)ids);
}

inline Id genTexture() {
	Id id;
	genTextures(1, &id);
	return id;
}

inline void deleteTextures(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteTextures((GLsizei)amount, ids);
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

inline void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteTextures(1, &id);
}

inline void genFramebuffers(uint8_t amount, Id* ids) {
	glGenFramebuffers((GLsizei)amount, (GLuint*)ids);
}

inline Id genFramebuffer() {
	Id id;
	genFramebuffers(1, &id);
	return id;
}

inline void deleteFramebuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteFramebuffers((GLsizei)amount, ids);
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

inline void deleteFramebuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteFramebuffers(1, &id);
}

inline void genRenderbuffers(uint8_t amount, Id* ids) {
	glGenRenderbuffers((GLsizei)amount, (GLuint*)ids);
}

inline Id genRenderbuffer() {
	Id id;
	genRenderbuffers(1, &id);
	return id;
}

inline void deleteRenderbuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteRenderbuffers((GLsizei)amount, ids);
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

inline void deleteRenderbuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteRenderbuffers(1, &id);
}

inline void configureAttribute(const Attribute& a) {
	glEnableVertexAttribArray(a.index);
	if (a.typeIsInt) {
		glVertexAttribIPointer(a.index, a.size, std::enum_value(a.type), a.stride, GL_OFFSET_CAST(a.offset));
	} else {
		glVertexAttribPointer(a.index, a.size, std::enum_value(a.type), a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
	}
	if (a.divisor > 0) {
		glVertexAttribDivisor(a.index, a.divisor);
	}
}

inline bool bindFramebuffer(FrameBufferMode mode, Id handle) {
	glBindFramebuffer(_priv::FrameBufferModes[std::enum_value(mode)], handle);
	return true;
}

inline bool bindRenderbuffer(Id handle) {
	glBindRenderbuffer(GL_RENDERBUFFER, handle);
	return true;
}

inline void bufferData(VertexBufferType type, VertexBufferMode mode, const void* data, size_t size) {
	glBufferData(_priv::VertexBufferTypes[std::enum_value(type)], (GLsizeiptr)size, data, _priv::VertexBufferModes[std::enum_value(mode)]);
}

inline void bufferSubData(VertexBufferType type, intptr_t offset, const void* data, size_t size) {
	glBufferSubData(_priv::VertexBufferTypes[std::enum_value(type)], (GLintptr)offset, (GLsizeiptr)size, data);
}

inline void disableDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

inline void setupDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
}

inline void setupTexture(TextureUnit unit, video::TextureType type, video::TextureWrap wrap, Id texture) {
	bindTexture(unit, type, texture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	const GLenum glWrap = _priv::TextureWraps[std::enum_value(wrap)];
	glTexParameteri(glType, GL_TEXTURE_WRAP_S, glWrap);
	glTexParameteri(glType, GL_TEXTURE_WRAP_T, glWrap);
	glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);
}

inline void uploadTexture(video::TextureType type, video::TextureWrap wrap, video::TextureFormat format, int width, int height, const uint8_t* data, int index) {
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	if (type == TextureType::Texture2D) {
		glTexImage2D(glType, 0, f.internalFormat, width, height, 0, f.dataFormat, f.dataType, (const void*)data);
	} else {
		glTexImage3D(glType, 0, f.internalFormat, width, height, index, 0, f.dataFormat, f.dataType, (const void*)data);
	}
}

template<class IndexType>
inline void drawElements(Primitive mode, size_t numIndices) {
	glDrawElements(std::enum_value(mode), (GLsizei)numIndices, std::enum_value(mapType<IndexType>()), nullptr);
}

template<class IndexType>
inline void drawElementsInstanced(Primitive mode, size_t numIndices, size_t amount) {
	glDrawElementsInstanced(std::enum_value(mode), (GLsizei)numIndices, std::enum_value(mapType<IndexType>()), nullptr, (GLsizei)amount);
}

template<class IndexType>
inline void drawElementsBaseVertex(Primitive mode, size_t numIndices, int baseIndex, int baseVertex) {
	glDrawElementsBaseVertex(std::enum_value(mode), (GLsizei)numIndices, std::enum_value(mapType<IndexType>()), (const void*)(sizeof(IndexType) * baseIndex), (GLint)baseVertex);
}

inline void drawArrays(Primitive mode, size_t count) {
	glDrawArrays(std::enum_value(mode), (GLint)0, (GLsizei)count);
}

}
