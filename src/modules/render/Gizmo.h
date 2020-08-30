/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "command/ActionButton.h"
#include "Axis.h"
#include "video/Ray.h"
#include "video/Camera.h"
#include <stdint.h>

namespace video {
class Video;
}

namespace render {

enum class GizmoMode : int8_t {
	None = 0,
	TranslateX, TranslateY, TranslateZ,
	Max
};

/**
 * @brief A set of manipulator handles in the shape of a 3 axis coordinate system icon
 * used for manipulating objects in 3D space
 *
 * @note This also implements a command::ActionButton
 */
class Gizmo : public core::IComponent, public command::ActionButton {
	// action button related stuff
private:
	double _buttonLastAction = 0.0;
	GizmoMode _buttonMode = GizmoMode::None;
	glm::vec3 _buttonLastPosition { 0.0f };
	glm::ivec2 _pixelPos { -1 };
	video::Ray _ray { glm::vec3(0.0f), glm::vec3(0.0f) };
public:
	bool handleDown(int32_t key, double pressedMillis) override;
	bool handleUp(int32_t key, double releasedMillis) override;

	// gizmo states
private:
	render::Axis _axis;
	GizmoMode _mode = GizmoMode::None;
	glm::vec3 _pos { 0.0f };
	bool _modelSpace = true;
	video::Camera _camera;

	bool calculateTranslationDelta(glm::vec3& delta);
	void updateTranslateState();
	bool isMode(GizmoMode mode) const;

public:
	bool isModelSpace() const;
	bool isWorldSpace() const;
	void setModelSpace();
	void setWorldSpace();

	/**
	 * @brief Test whether the given position hits and of the axes of the rendered geometry and set
	 * the internal mode to reflect that hit.
	 *
	 * @param[in] camera The camera to take the matrix from
	 * @param[in] pixelPos screen pixel position
	 */
	void updateMode(const video::Camera& camera, const glm::ivec2& pixelPos);
	void resetMode();
	/**
	 * @return The current selected Gizmo::Mode value
	 * @note This is the mode that is currently active in the action button. That means
	 * that the button must be triggered.
	 */
	GizmoMode mode() const;

	/**
	 * @brief Updates the origin of the gizmo
	 */
	void setPosition(const glm::vec3& pos);

	/**
	 * @brief Renders the handles of the gizmo
	 */
	void render(const video::Camera& camera);

	/**
	 * @return @c true on success, @c false on failure
	 * @sa shutdown()
	 */
	bool init() override;
	/**
	 * @sa init()
	 */
	void shutdown() override;

	/**
	 * @brief Tries to execute the action button
	 */
	bool execute(double nowSeconds, const std::function<void(const glm::vec3, GizmoMode)>& function);
};

inline bool Gizmo::isModelSpace() const {
	return _modelSpace;
}

inline bool Gizmo::isWorldSpace() const {
	return !isModelSpace();
}

inline void Gizmo::setModelSpace() {
	_modelSpace = true;
}

inline void Gizmo::setWorldSpace() {
	_modelSpace = false;
}

}
