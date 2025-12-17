/**
 * @file
 */

#pragma once

#include "color/Color.h"
#include "core/Common.h"
#include "render/ShapeRenderer.h"
#include "video/Camera.h"
#include "video/ShapeBuilder.h"

namespace render {

/**
 * @brief Renders a video::Camera math::Frustum
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class CameraRenderer {
protected:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	int _splitFrustum = -1;
	int32_t _frustumMesh = -1;
	int32_t _aabbMesh = -1;
	bool _renderAABB = false;

public:
	struct Node {
		int nodeId;
		color::RGBA color;
		video::Camera camera;

		Node(int nid, video::Camera &&cam, color::RGBA col)
			: nodeId(nid), color(col.r, col.g, col.b), camera(core::move(cam)) {
		}
	};

	/**
	 * @param[in] splitFrustum The amount of splits that should be rendered.
	 */
	bool init(int splitFrustum = 0);
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

	/**
	 * @brief Renders the camera frustum and optionally its AABB.
	 */
	void render(const video::Camera &camera, const Node &frustumCamera);
};

inline void CameraRenderer::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

inline bool CameraRenderer::renderAABB() const {
	return _renderAABB;
}

} // namespace render
