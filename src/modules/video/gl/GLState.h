/**
 * @file
 */

#pragma once

#include "flextGL.h"
#include "GLVersion.h"
#include "video/Types.h"
#include <bitset>
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
	GLVersion glVersion {0, 0};
	glm::vec4 clearColor;
	Face cullFace = Face::Back;
	CompareFunc depthFunc = CompareFunc::Less;
	Id programHandle = InvalidId;
	Id vertexArrayHandle = InvalidId;
	glm::vec2 polygonOffset;
	Face polygonModeFace = Face::Max;
	PolygonMode polygonMode = PolygonMode::Solid;
	BlendMode blendSrc = BlendMode::One;
	BlendMode blendDest = BlendMode::Zero;
	BlendEquation blendEquation = BlendEquation::Max;
	TextureUnit textureUnit = TextureUnit::Zero;
	Id textureHandle = InvalidId;
	Id occlusionQuery = InvalidId;
	int viewportX = 0;
	int viewportY = 0;
	int viewportW = 0;
	int viewportH = 0;
	int scissorX = 0;
	int scissorY = 0;
	int scissorW = 0;
	int scissorH = 0;
	std::bitset<std::enum_value(State::Max)> states;
	Id bufferHandle[std::enum_value(VertexBufferType::Max)] = {};
	Id framebufferHandle = InvalidId;
	Id framebufferTextureHandle = InvalidId;
	glm::vec2 smoothedLineWidth = glm::vec2(-1.0f);
	glm::vec2 aliasedLineWidth = glm::vec2(-1.0f);
	float lineWidth = 1.0f;
	std::bitset<std::enum_value(Vendor::Max)> vendor;
};

static GLState s;

}

}
