#pragma once

#include "video/Camera.h"
#include "core/Color.h"
#include "core/Plane.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

namespace frontend {

/**
 * @brief Renders a plane
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Plane {
private:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	int32_t _planeMesh = -1;
public:
	void render(const video::Camera& camera) {
		video::disable(video::State::CullFace);
		_shapeRenderer.render(_planeMesh, camera);
		video::enable(video::State::CullFace);
	}

	void shutdown() {
		_shapeRenderer.shutdown();
		_shapeBuilder.shutdown();
	}

	/**
	 * @param[in] position The offset that should be applied to the center of the plane
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 * @param[in] scale The vertices are in the normalized coordinate space between -0.5 and 0.5 - we have to scale them up to the size we need
	 * @param[in] color The color of the plane.
	 */
	bool init() {
		if (!_shapeRenderer.init()) {
			return false;
		}
		return true;
	}

	bool plane(const glm::vec3& position, int tesselation = 0, float scale = 100.0f, const glm::vec4& color = core::Color::White) {
		_shapeBuilder.setColor(color);
		_shapeBuilder.setPosition(position);
		_shapeBuilder.plane(tesselation, scale);
		_shapeRenderer.createOrUpdate(_planeMesh, _shapeBuilder);
		return _planeMesh >= 0;
	};

	bool plane(const glm::vec3& position, const core::Plane& plane, const glm::vec4& color = core::Color::White) {
		_shapeBuilder.setColor(color);
		_shapeBuilder.setPosition(position);
		_shapeBuilder.plane(plane, false);
		_shapeRenderer.createOrUpdate(_planeMesh, _shapeBuilder);
		return _planeMesh >= 0;
	};
};

}
