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
	};
	static GLState s;
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

	glEnable(std::enum_value(state));
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

	glDisable(std::enum_value(state));
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
	glDepthFunc(std::enum_value(func));
	_priv::s.depthFunc = func;
	return true;
}

inline bool blendFunc(BlendMode src, BlendMode dest) {
	if (_priv::s.blendSrc == src && _priv::s.blendDest == dest) {
		return false;
	}
	_priv::s.blendSrc = src;
	_priv::s.blendDest = dest;
	glBlendFunc(std::enum_value(src), std::enum_value(dest));
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
		glBindTexture(std::enum_value(type), handle);
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
	glBindBuffer(std::enum_value(type), handle);
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
	glBindFramebuffer(std::enum_value(mode), handle);
	return true;
}

inline bool bindRenderbuffer(Id handle) {
	glBindRenderbuffer(GL_RENDERBUFFER, handle);
	return true;
}

inline void bufferData(VertexBufferType type, VertexBufferMode mode, const void* data, size_t size) {
	glBufferData(std::enum_value(type), (GLsizeiptr)size, data, std::enum_value(mode));
}

inline void bufferSubData(VertexBufferType type, intptr_t offset, const void* data, size_t size) {
	glBufferSubData(std::enum_value(type), (GLintptr)offset, (GLsizeiptr)size, data);
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
