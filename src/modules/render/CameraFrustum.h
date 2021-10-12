/**
 * @file
 */

#pragma once

#include "render/ShapeRenderer.h"
#include "core/Common.h"
#include "core/Color.h"
#include "video/ShapeBuilder.h"

namespace video {
class Video;
}

namespace render {

/**
 * @brief Renders a video::Camera math::Frustum
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class CameraFrustum {
protected:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	glm::vec4 _color;
	int _splitFrustum = -1;
	int32_t _frustumMesh = -1;
	int32_t _aabbMesh = -1;
	bool _renderAABB = false;
public:
	/**
	 * @param[in] splitFrustum The amount of splits that should be rendered.
	 */
	bool init(const video::Camera& frustumCamera, const glm::vec4& color = core::Color::Red, int splitFrustum = 0);

	void shutdown();

	/**
	 * @param[in] renderAABB @c true if the math::AABB should be rendered for the camera frustum. @c false if not
	 * @see renderAABB()
	 */
	void setRenderAABB(bool renderAABB);
	/**
	 * @see setRenderAABB()
	 */
	bool renderAABB() const;

	void render(const video::Camera& camera, const video::Camera& frustumCamera);
};

inline void CameraFrustum::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

inline bool CameraFrustum::renderAABB() const {
	return _renderAABB;
}

}
