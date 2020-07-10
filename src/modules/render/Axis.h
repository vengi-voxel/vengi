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

namespace AxisMask {
static const uint32_t RenderXAxis = 1 << 0;
static const uint32_t RenderXCone = 1 << 1;
static const uint32_t RenderX = RenderXAxis | RenderXCone;

static const uint32_t RenderYAxis = 1 << 2;
static const uint32_t RenderYCone = 1 << 3;
static const uint32_t RenderY = RenderYAxis | RenderYCone;

static const uint32_t RenderZAxis = 1 << 4;
static const uint32_t RenderZCone = 1 << 5;
static const uint32_t RenderZ = RenderZAxis | RenderZCone;

static const uint32_t RenderAll = RenderX | RenderY | RenderZ;
static const uint32_t RenderAxes = RenderXAxis | RenderYAxis | RenderZAxis;
}

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
	int32_t _meshXIndex = -1;
	int32_t _meshYIndex = -1;
	int32_t _meshZIndex = -1;
	int32_t _coneXIndex = -1;
	int32_t _coneYIndex = -1;
	int32_t _coneZIndex = -1;
	float _lineWidth = 4.0f;
	glm::vec3 _pos { 0.0f };
	glm::vec3 _size { 1.0f };
	const bool _flipZ;

	void createCones();

public:
	Axis(bool flipZ = true);

	void setSize(float x, float y, float z);
	void setPosition(const glm::vec3& pos);

	void render(const video::Camera& camera, uint32_t renderMask = AxisMask::RenderAll);

	bool init() override;
	void shutdown() override;
};

}
