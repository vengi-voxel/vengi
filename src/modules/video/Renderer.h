#pragma once

#include "Types.h"
#include "image/Image.h"

namespace video {

/**
 * @brief Maps data types to GL enums
 */
template<class DATATYPE>
constexpr inline DataType mapType() {
	if (std::is_floating_point<DATATYPE>()) {
		return DataType::Float;
	}

	if (sizeof(DATATYPE) == 1u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedByte;
		}
		return DataType::Byte;
	}

	if (sizeof(DATATYPE) == 2u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedShort;
		}
		return DataType::Short;
	}

	if (sizeof(DATATYPE) == 4u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedInt;
		}
		return DataType::Int;
	}

	return DataType::Max;
}

extern void checkError();
extern bool setupCubemap(Id handle, const image::ImagePtr images[6]);
extern void readBuffer(GBufferTextureType textureType);
extern bool setupDepthbuffer(Id fbo, DepthBufferMode mode);
extern bool bindDepthTexture(int textureIndex, DepthBufferMode mode, Id depthTexture);
extern bool setupGBuffer(Id fbo, const glm::ivec2& dimension, Id* textures, int texCount, Id depthTexture);
extern float lineWidth(float width);
extern bool clearColor(const glm::vec4& clearColor);
extern void clear(ClearFlag flag);
extern bool viewport(int x, int y, int w, int h);
extern void getViewport(int& x, int& y, int& w, int& h);
extern bool scissor(int x, int y, int w, int h);
extern bool enable(State state);
extern bool disable(State state);
extern bool cullFace(Face face);
extern bool depthFunc(CompareFunc func);
extern bool blendFunc(BlendMode src, BlendMode dest);
extern bool polygonMode(Face face, PolygonMode mode);
extern bool polygonOffset(const glm::vec2& offset);
extern bool bindTexture(TextureUnit unit, TextureType type, Id handle);
extern bool useProgram(Id handle);
extern bool bindVertexArray(Id handle);
extern bool bindBuffer(VertexBufferType type, Id handle);
extern bool bindBufferBase(VertexBufferType type, Id handle, uint32_t index = 0u);
extern void genBuffers(uint8_t amount, Id* ids);
extern Id genBuffer();
extern void deleteBuffers(uint8_t amount, Id* ids);
extern void deleteBuffer(Id& id);
extern void genVertexArrays(uint8_t amount, Id* ids);
extern Id genVertexArray();
extern void deleteShader(Id& id);
extern Id genShader(ShaderType type);
extern void deleteProgram(Id& id);
extern Id genProgram();
extern void deleteVertexArrays(uint8_t amount, Id* ids);
extern void deleteVertexArray(Id& id);
extern void genTextures(uint8_t amount, Id* ids);
extern Id genTexture();
extern void deleteTextures(uint8_t amount, Id* ids);
extern void deleteTexture(Id& id);
extern void genFramebuffers(uint8_t amount, Id* ids);
extern Id genFramebuffer();
extern void deleteFramebuffers(uint8_t amount, Id* ids);
extern void deleteFramebuffer(Id& id);
extern void genRenderbuffers(uint8_t amount, Id* ids);
extern Id genRenderbuffer();
extern void deleteRenderbuffers(uint8_t amount, Id* ids);
extern void deleteRenderbuffer(Id& id);
extern void configureAttribute(const Attribute& a);
extern Id bindFramebuffer(FrameBufferMode mode, Id handle, Id textureHandle = InvalidId);
extern bool bindRenderbuffer(Id handle);
extern void bufferData(VertexBufferType type, VertexBufferMode mode, const void* data, size_t size);
extern void bufferSubData(VertexBufferType type, intptr_t offset, const void* data, size_t size);
extern void disableDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture);
extern void setupDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture);
extern bool setupFramebuffer(Id& _fbo, Id& _texture, Id& _depth, const glm::ivec2& dimension);
extern void setupTexture(video::TextureType type, video::TextureWrap wrap);
extern void uploadTexture(video::TextureType type, video::TextureFormat format, int width, int height, const uint8_t* data, int index);
extern void drawElements(Primitive mode, size_t numIndices, DataType type);
extern void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount);
extern void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex);
extern void drawArrays(Primitive mode, size_t count);
extern void enableDebug(DebugSeverity severity);

template<class IndexType>
inline void drawElements(Primitive mode, size_t numIndices) {
	drawElements(mode, numIndices, mapType<IndexType>());
}

template<class IndexType>
inline void drawElementsInstanced(Primitive mode, size_t numIndices, size_t amount) {
	drawElementsInstanced(mode, numIndices, mapType<IndexType>(), amount);
}

template<class IndexType>
inline void drawElementsBaseVertex(Primitive mode, size_t numIndices, int baseIndex, int baseVertex) {
	drawElementsBaseVertex(mode, numIndices, mapType<IndexType>(), sizeof(IndexType), baseIndex, baseVertex);
}

}
