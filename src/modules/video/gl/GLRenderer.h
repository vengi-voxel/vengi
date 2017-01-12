#pragma once

#include "GLTypes.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

namespace _priv {
	struct GLState {
		glm::vec4 clearColor;
		bool depthMask;
		Face cullFace;
		CompareFunc depthFunc;
		Id programHandle;
		glm::vec2 polygonOffset;
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

inline bool enable(State state) {
	if (state == State::DepthMask) {
		if (_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_TRUE);
		_priv::s.depthMask = true;
	} else {
		glEnable(std::enum_value(state));
	}
	return true;
}

inline bool disable(State state) {
	if (state == State::DepthMask) {
		if (!_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_FALSE);
		_priv::s.depthMask = false;
	} else {
		glDisable(std::enum_value(state));
	}
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
	glBlendFunc(std::enum_value(src), std::enum_value(dest));
	return true;
}

template<class IndexType>
inline void drawElements(Primitive mode, size_t numIndices) {
	glDrawElements(std::enum_value(mode), (GLsizei)numIndices, GLmap<IndexType>(), nullptr);
}

template<class IndexType>
inline void drawElementsInstanced(Primitive mode, size_t numIndices, size_t amount) {
	glDrawElementsInstanced(std::enum_value(mode), (GLsizei)numIndices, GLmap<IndexType>(), nullptr, (GLsizei)amount);
}

template<class IndexType>
inline void drawElementsBaseVertex(Primitive mode, size_t numIndices, int baseIndex, int baseVertex) {
	glDrawElementsBaseVertex(std::enum_value(mode), (GLsizei)numIndices, GLmap<IndexType>(), (const void*)(sizeof(IndexType) * baseIndex), (GLint)baseVertex);
}

inline void drawArrays(Primitive mode, size_t count) {
	glDrawArrays(std::enum_value(mode), (GLint)0, (GLsizei)count);
}

inline bool polygonMode(Face face, PolygonMode mode) {
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

inline bool useProgram(Id handle) {
	if (_priv::s.programHandle == handle) {
		return false;
	}
	glUseProgram(handle);
	_priv::s.programHandle = handle;
	return true;
}

}
