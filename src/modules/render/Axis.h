/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/IComponent.h"
#include "math/Axis.h"

namespace video {
class Camera;
}

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
	int32_t _meshIndex = -1;
	int32_t _coneXIndex = -1;
	int32_t _coneYIndex = -1;
	int32_t _coneZIndex = -1;
	float _lineWidth = 4.0f;
	glm::vec3 _pos { 0.0f };
	glm::vec3 _size { 1.0f };
	const bool _flipZ;
	const bool _arrowHeads;

	void createCones();

public:
	Axis(bool flipZ = true, bool arrowHeads = true);

	void setSize(float x, float y, float z);
	void setPosition(const glm::vec3& pos);

	void render(const video::Camera& camera);

	bool init() override;
	void shutdown() override;
};

}
