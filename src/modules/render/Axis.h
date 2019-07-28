/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#include "core/IComponent.h"

namespace render {

/**
 * @brief Renders a world axis (xyz)
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Axis : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	float _lineWidth = 4.0f;
	glm::vec3 _pos = glm::zero<glm::vec3>();
public:
	Axis();

	void setPosition(const glm::vec3& pos);

	void render(const video::Camera& camera);

	bool init() override;
	void shutdown() override;
};

}
