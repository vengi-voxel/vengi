/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "core/Var.h"
#include "video/Camera.h"
#include <SDL.h>

namespace voxedit {

class Controller {
public:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front
	};

private:
	float _angle = 0.0f;
	float _cameraSpeed = 0.1f;
	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;

public:
	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	void init(Controller::SceneCameraMode mode);
	void resetCamera(const voxel::Region& region);

	void onResize(const glm::ivec2& size);

	void update(long deltaFrame);

	bool move(bool rotate, int x, int y);
	void zoom(float level);

	video::Camera& camera();

	float angle() const;
	void setAngle(float angle);

	float cameraSpeed() const;
	void setCameraSpeed(float cameraSpeed);
};

inline float Controller::cameraSpeed() const {
	return _cameraSpeed;
}

inline void Controller::setCameraSpeed(float cameraSpeed) {
	_cameraSpeed = cameraSpeed;
}

inline video::Camera& Controller::camera() {
	return _camera;
}

inline float Controller::angle() const {
	return _angle;
}

inline void Controller::setAngle(float angle) {
	_angle = angle;
}

}
