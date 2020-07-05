/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/command/ActionButton.h"
#include "Axis.h"

namespace video {
class Video;
}

namespace render {

enum class GizmoMode {
	None, TranslateX, TranslateY, TranslateZ,
	/* TODO: Rotate, Scale,*/
	Max
};
/**
 * @brief A set of manipulator handles in the shape of a 3 axis coordinate system icon
 * used for manipulating objects in 3D space
 *
 * @note This also implements a core::ActionButton
 */
class Gizmo : public core::IComponent, public core::ActionButton {
	// action button related stuff
private:
	double _buttonLastAction = 0.0;
	GizmoMode _buttonMode = GizmoMode::None;
	glm::ivec3 _buttonLastPosition { 0 };
public:
	bool handleDown(int32_t key, double pressedMillis) override;
	bool handleUp(int32_t key, double releasedMillis) override;

	// gizmo states
private:
	render::Axis _axis;
	GizmoMode _mode = GizmoMode::None;
	glm::vec3 _pos { 0.0f };
	bool _modelSpace = true;

public:
	/**
	 * @return The current selected Gizmo::Mode value
	 * @note update() must have been called before
	 */
	GizmoMode mode() const;
	void setMode(GizmoMode mode);

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
	void update(const video::Camera& camera, const glm::ivec2& pixelPos);

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
	bool execute(double nowSeconds, const std::function<glm::ivec3(const glm::ivec3, GizmoMode)>& function);
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

inline void Gizmo::setMode(GizmoMode mode) {
	_mode = mode;
}

}
