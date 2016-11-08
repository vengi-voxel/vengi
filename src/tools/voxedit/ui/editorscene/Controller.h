#pragma once

class Controller {
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
};
