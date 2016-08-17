#pragma once

#include "core/Common.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "ColorShader.h"

namespace frontend {

class CameraFrustum {
protected:
	video::VertexBuffer _frustumBuffer;
	int32_t _vertexIndex = -1;
	int32_t _indexIndex = -1;
	shader::ColorShader _colorShader;
public:
	bool init(const video::Camera& frustumCamera, const glm::vec4& color = core::Color::Red);

	void shutdown();

	void render(const video::Camera& camera, const video::Camera& frustumCamera);
};

}
