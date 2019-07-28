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
#include "math/Axis.h"

namespace render {

/**
 * @brief Renders a world axis (xyz)
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Axis : public core::IComponent {
public:
	enum class Mode {
		None, TranslateX, TranslateY, TranslateZ,
		/* TODO: Rotate, Scale,*/
		Max
	};
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _meshIndex = -1;
	float _lineWidth = 4.0f;
	Mode _mode = Mode::None;
	glm::vec3 _pos = glm::zero<glm::vec3>();

public:
	Axis();
	Mode mode() const;

	void setPosition(const glm::vec3& pos);

	void render(const video::Camera& camera);

	/**
	 * @brief Test whether the given position hits and of the axes of the rendered geometry and set
	 * the internal mode to reflect that hit.
	 *
	 * @param[in] camera The camera to take the matrix from
	 * @param[in] pixelPos screen pixel position
	 */
	void update(const video::Camera& camera, const glm::vec2& pixelPos);

	bool init() override;
	void shutdown() override;
};

inline Axis::Mode Axis::mode() const {
	return _mode;
}

}
