#pragma once

#include "video/Camera.h"
#include "core/Color.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

namespace frontend {

class Plane {
private:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	int32_t _planeMesh = -1;
public:
	void render(const video::Camera& camera) {
		_shapeRenderer.render(_planeMesh, camera, GL_TRIANGLES);
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
	bool init(const glm::vec3& position, int tesselation = 0, float scale = 100.0f, const glm::vec4& color = core::Color::White) {
		if (!_shapeRenderer.init()) {
			return false;
		}
		_shapeBuilder.setColor(color);
		_shapeBuilder.setPosition(position);
		_shapeBuilder.plane(tesselation, scale);
		_planeMesh = _shapeRenderer.createMesh(_shapeBuilder);
		return _planeMesh >= 0;
	};
};

}
