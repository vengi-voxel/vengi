#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "core/Var.h"
#include "video/Camera.h"
#include <SDL.h>

class Controller {
private:
	float _angle = 0.0f;

	inline bool isRelativeMouseMode() const {
		return SDL_GetRelativeMouseMode() == SDL_TRUE ? true : false;
	}

	inline bool isMiddleMouseButtonPressed() const {
		return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_MIDDLE);
	}

	inline bool isRightMouseButtonPressed() const {
		return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT);
	}

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
	video::Camera _camera;
	core::VarPtr _rotationSpeed;
	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	enum class SceneCameraMode {
		Free, Top, Left, Front
	};

	SceneCameraMode _camMode = SceneCameraMode::Free;

	uint8_t _moveMask = 0;

	float _cameraSpeed = 0.1f;

	void init(Controller::SceneCameraMode mode);
	void resetCamera(const voxel::RawVolume* volume);

	void onResize(const glm::ivec2& pos, const glm::ivec2& size);

	void update(long deltaFrame);

	bool move(bool rotate, int x, int y);
	void zoom(float level);

	video::Camera& camera();

	float angle() const;
	void setAngle(float angle);
};

inline video::Camera& Controller::camera() {
	return _camera;
}

inline float Controller::angle() const {
	return _angle;
}

inline void Controller::setAngle(float angle) {
	_angle = angle;
}
