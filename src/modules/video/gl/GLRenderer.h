#pragma once

#include "GLTypes.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

inline void clearColor(const glm::vec4& clearColor) {
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
}

inline void clear(ClearFlag flag) {
	glClear(std::enum_value(flag));
}

inline bool enable(State state) {
	if (state == State::DepthMask) {
		glDepthMask(GL_TRUE);
	} else {
		glEnable(std::enum_value(state));
	}
	return true;
}

inline bool disable(State state) {
	if (state == State::DepthMask) {
		glDepthMask(GL_TRUE);
	} else {
		glDisable(std::enum_value(state));
	}
	return true;
}

inline void cullFace(Face face) {
	glCullFace(std::enum_value(face));
}

inline bool depthFunc(CompareFunc func) {
	glDepthFunc(std::enum_value(func));
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
	glPolygonOffset(offset.x, offset.y);
	return true;
}

}
