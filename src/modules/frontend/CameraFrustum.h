#pragma once

#include "frontend/ShapeRenderer.h"
#include "core/Common.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "video/ShapeBuilder.h"
#include "ColorShader.h"

namespace frontend {

class CameraFrustum {
protected:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;

	int32_t _frustumMesh;
	int32_t _aabbMesh;
	bool _renderAABB = false;
public:
	bool init(const video::Camera& frustumCamera, const glm::vec4& color = core::Color::Red);

	void shutdown();

	void setRenderAABB(bool renderAABB);

	void render(const video::Camera& camera, const video::Camera& frustumCamera);
};

inline void CameraFrustum::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

}
