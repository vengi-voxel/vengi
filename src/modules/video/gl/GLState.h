#pragma once

#include "GLFunc.h"
#include "GLVersion.h"
#include "video/Types.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace video {

namespace _priv {

struct GLState {
	GLVersion glVersion {0, 0};
	int limits[std::enum_value(video::Limit::Max)] = { };
	bool features[std::enum_value(video::Feature::Max)] = { };
	glm::vec4 clearColor;
	bool depthMask = true;
	Face cullFace = Face::Max;
	CompareFunc depthFunc = CompareFunc::Max;
	Id programHandle = InvalidId;
	Id vertexArrayHandle = InvalidId;
	glm::vec2 polygonOffset;
	Face polygonModeFace = Face::Max;
	PolygonMode polygonMode = PolygonMode::Max;
	BlendMode blendSrc = BlendMode::SourceAlpha;
	BlendMode blendDest = BlendMode::OneMinusSourceAlpha;
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
	Id bufferHandle[std::enum_value(VertexBufferType::Max)] = {};
	Id bufferBaseHandle[std::enum_value(VertexBufferType::Max)] = {};
	Id framebufferHandle = InvalidId;
	Id framebufferTextureHandle = InvalidId;
	glm::vec2 smoothedLineWidth = glm::vec2(-1.0f);
	glm::vec2 aliasedLineWidth = glm::vec2(-1.0f);
	bool lineAntialiasing = false;
	float lineWidth = 0.0f;
};

static GLState s;

}

}
