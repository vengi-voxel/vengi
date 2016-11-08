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
	uint8_t _moveMask = 0;
	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;

public:
#if 0
	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
#endif
	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	void init(Controller::SceneCameraMode mode);
	void resetCamera(const voxel::RawVolume* volume);
	void changeCameraRotationType(video::CameraRotationType type);

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
