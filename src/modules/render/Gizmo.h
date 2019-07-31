/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/command/ActionButton.h"
#include "Axis.h"

namespace render {

class Gizmo : public core::IComponent, public core::ActionButton {
public:
	enum class Mode {
		None, TranslateX, TranslateY, TranslateZ,
		/* TODO: Rotate, Scale,*/
		Max
	};
private:
	render::Axis _axis;
	Mode _mode = Mode::None;
	glm::vec3 _pos = glm::zero<glm::vec3>();

	// action button related stuff
	uint64_t _buttonLastAction = 0;
	render::Gizmo::Mode _buttonMode = render::Gizmo::Mode::None;
	glm::ivec3 _buttonLastPosition { 0 };
	bool handleDown(int32_t key, uint64_t pressedMillis) override;
	bool handleUp(int32_t key, uint64_t releasedMillis) override;

public:
	Mode mode() const;

	/**
	 * @brief Test whether the given position hits and of the axes of the rendered geometry and set
	 * the internal mode to reflect that hit.
	 *
	 * @param[in] camera The camera to take the matrix from
	 * @param[in] pixelPos screen pixel position
	 */
	void update(const video::Camera& camera, const glm::ivec2& pixelPos);

	void setPosition(const glm::vec3& pos);

	void render(const video::Camera& camera);

	bool init() override;
	void shutdown() override;
public:
	/**
	 * @brief Tries to execute the action button
	 */
	bool execute(uint64_t time, std::function<glm::ivec3(const glm::ivec3, render::Gizmo::Mode)> function);

};

}
