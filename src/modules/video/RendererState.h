/**
 * @file
 */

#pragma once

#include "core/collection/DynamicSet.h"
#include "core/collection/BitSet.h"
#include "core/collection/Map.h"
#include "video/Types.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

/**
 * Record the current opengl states to perform lesser state changes on the hardware.
 *
 * A list of default gl states: http://www.glprogramming.com/red/appendixb.html
 */
struct RendererState {
	core::DynamicSet<Id> textures;
	bool clipOriginLowerLeft = true;
	glm::vec4 clearColor {0.0f};
	glm::vec4 pendingClearColor {0.0f};
	Face cullFace = Face::Back;
	Face pendingCullFace = Face::Back;
	CompareFunc depthFunc = CompareFunc::Less;
	CompareFunc pendingDepthFunc = CompareFunc::Less;
	CompareFunc stencilFunc = CompareFunc::Always;
	StencilOp stencilOpFail = StencilOp::Keep;
	StencilOp stencilOpZfail = StencilOp::Keep;
	StencilOp stencilOpZpass = StencilOp::Keep;
	uint32_t stencilMask = 0xFFFFFFFF;
	uint32_t stencilValue = 0;
	Id programHandle = InvalidId;
	Id pendingProgramHandle = InvalidId;
	bool needValidation = false;
	Id vertexArrayHandle = InvalidId;
	glm::vec2 polygonOffset {0.0f};
	glm::vec2 pendingPolygonOffset {0.0f};
	float pointSize = 1.0f;
	float pendingPointSize = 1.0f;
	Face polygonModeFace = Face::Max;
	Face pendingPolygonModeFace = Face::Max;
	PolygonMode polygonMode = PolygonMode::Solid;
	PolygonMode pendingPolygonMode = PolygonMode::Solid;
	BlendMode blendSrcRGB = BlendMode::Max;
	BlendMode blendDestRGB = BlendMode::Max;
	BlendMode blendSrcAlpha = BlendMode::Max;
	BlendMode blendDestAlpha = BlendMode::Max;
	BlendEquation blendEquation = BlendEquation::Max;
	BlendMode pendingBlendSrcRGB = BlendMode::Max;
	BlendMode pendingBlendDestRGB = BlendMode::Max;
	BlendMode pendingBlendSrcAlpha = BlendMode::Max;
	BlendMode pendingBlendDestAlpha = BlendMode::Max;
	BlendEquation pendingBlendEquation = BlendEquation::Max;
	TextureUnit textureUnit = TextureUnit::Zero;
	Id textureHandle[core::enumVal(TextureUnit::Max)] { InvalidId };
	Id pendingTextureHandle[core::enumVal(TextureUnit::Max)] { InvalidId };
	TextureType pendingTextureType[core::enumVal(TextureUnit::Max)] { TextureType::Max };
	Id imageHandle = InvalidId;
	AccessMode imageAccessMode = AccessMode::Max;
	ImageFormat imageFormat = ImageFormat::Max;
	Id occlusionQuery = InvalidId;
	Id transformFeedback = InvalidId;
	int viewportX = 0;
	int viewportY = 0;
	int viewportW = 0;
	int viewportH = 0;
	int pendingViewportX = 0;
	int pendingViewportY = 0;
	int pendingViewportW = 0;
	int pendingViewportH = 0;
	int windowWidth = 0;
	int windowHeight = 0;
	/**
	 * These functions use pixel dimensions:
	 * * glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
	 * * glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
	 * * glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, ...)
	 * * glLineWidth(GLfloat width)
	 * * glRenderbufferStorage(..., GLsizei width, GLsizei height)
	 * * glTexImage2D(..., GLsizei width, GLsizei height, ...)
	 * * glBlitFramebuffer
	 * * glPointSize
	 */
	float scaleFactor = 1.0f;
	int scissorX = 0;
	int scissorY = 0;
	int scissorW = 0;
	int scissorH = 0;
	int pendingScissorX = 0;
	int pendingScissorY = 0;
	int pendingScissorW = 0;
	int pendingScissorH = 0;
	core::BitSet<core::enumVal(State::Max)> states;
	core::BitSet<core::enumVal(State::Max)> pendingStates;
	bool colorMask[4] {true, true, true, true};
	bool pendingColorMask[4] {true, true, true, true};
	Id bufferHandle[core::enumVal(BufferType::Max)] = {};
	Id framebufferHandle = InvalidId;
	FrameBufferMode framebufferMode = FrameBufferMode::Default;
	Id renderBufferHandle = InvalidId;
	glm::vec2 smoothedLineWidth = glm::vec2(-1.0f);
	glm::vec2 aliasedLineWidth = glm::vec2(-1.0f);
	float lineWidth = 1.0f;
	float pendingLineWidth = 1.0f;
	core::BitSet<core::enumVal(Vendor::Max)> vendor;
	int drawCalls = 0;
	// Cache for uniform buffer bindings: maps (program << 32 | blockIndex) to blockBinding
	core::Map<uint64_t, uint32_t, 64> uniformBufferBindings;
	// Cache for buffer base bindings: maps (type << 32 | index) to buffer handle
	core::Map<uint64_t, Id, 64> bufferBaseBindings;
	// Pending integer uniforms: maps location to value (deferred until draw calls)
	core::Map<int, int, 32> pendingUniformi;
};

}
