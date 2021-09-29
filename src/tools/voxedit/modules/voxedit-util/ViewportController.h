/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/Var.h"
#include "video/Camera.h"

namespace voxedit {

/**
 * @brief These are the viewport settings that belong to each viewport instance.
 *
 * The camera and render settings are part of this instance.
 *
 * @sa Viewport
 */
class ViewportController {
public:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front
	};

	enum class RenderMode {
		Editor,
		Animation,

		Max
	};

private:
	float _angle = 0.0f;
	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;
	RenderMode _renderMode = RenderMode::Editor;
public:
	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	void init(SceneCameraMode mode);
	void resetCamera(const voxel::Region& region);

	RenderMode renderMode() const;
	void setRenderMode(RenderMode mode);

	void onResize(const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize);

	void update(double deltaFrameSeconds);

	void move(bool rotate, int x, int y);

	video::Camera& camera();

	float angle() const;
	void setAngle(float angle);
};

inline ViewportController::RenderMode ViewportController::renderMode() const {
	return _renderMode;
}

inline void ViewportController::setRenderMode(RenderMode renderMode) {
	_renderMode = renderMode;
}

inline video::Camera& ViewportController::camera() {
	return _camera;
}

inline float ViewportController::angle() const {
	return _angle;
}

inline void ViewportController::setAngle(float angle) {
	_angle = angle;
}

}
