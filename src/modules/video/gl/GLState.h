/**
 * @file
 */

#pragma once

#include "flextGL.h"
#include "GLVersion.h"
#include "video/Types.h"
#include <bitset>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

namespace _priv {

/**
 * Record the current opengl states to perform lesser state changes on the hardware.
 *
 * A list of default gl states: http://www.glprogramming.com/red/appendixb.html
 */
struct GLState {
	bool clipOriginLowerLeft = true;
	GLVersion glVersion {0, 0};
	glm::vec4 clearColor {0.0f};
	Face cullFace = Face::Back;
	CompareFunc depthFunc = CompareFunc::Less;
	CompareFunc stencilFunc = CompareFunc::Always;
	StencilOp stencilOpFail = StencilOp::Keep;
	StencilOp stencilOpZfail = StencilOp::Keep;
	StencilOp stencilOpZpass = StencilOp::Keep;
	uint32_t stencilMask = 0xFFFFFFFF;
	uint32_t stencilValue = 0;
	Id programHandle = InvalidId;
	Id vertexArrayHandle = InvalidId;
	glm::vec2 polygonOffset {0.0f};
	Face polygonModeFace = Face::Max;
	PolygonMode polygonMode = PolygonMode::Solid;
	BlendMode blendSrc = BlendMode::One;
	BlendMode blendDest = BlendMode::Zero;
	BlendEquation blendEquation = BlendEquation::Max;
	TextureUnit textureUnit = TextureUnit::Zero;
	Id textureHandle[std::enum_value(TextureUnit::Max)] { InvalidId };
	Id imageHandle = InvalidId;
	AccessMode imageAccessMode = AccessMode::Max;
	ImageFormat imageFormat = ImageFormat::Max;
	Id occlusionQuery = InvalidId;
	Id transformFeedback = InvalidId;
	int viewportX = 0;
	int viewportY = 0;
	int viewportW = 0;
	int viewportH = 0;
	int windowWidth = 0;
	int windowHeight = 0;
	float scaleFactor = 1.0f;
	int scissorX = 0;
	int scissorY = 0;
	int scissorW = 0;
	int scissorH = 0;
	std::bitset<std::enum_value(State::Max)> states;
	Id bufferHandle[std::enum_value(BufferType::Max)] = {};
	Id framebufferHandle = InvalidId;
	Id renderBufferHandle = InvalidId;
	glm::vec2 smoothedLineWidth = glm::vec2(-1.0f);
	glm::vec2 aliasedLineWidth = glm::vec2(-1.0f);
	float lineWidth = 1.0f;
	std::bitset<std::enum_value(Vendor::Max)> vendor;
};

static GLState s;

}

}
