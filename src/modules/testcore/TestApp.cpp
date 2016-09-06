#include "TestApp.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "core/Command.h"
#include "frontend/Movement.h"

TestApp::TestApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus, 21000) {
	init("engine", "test");
}

TestApp::~TestApp() {
}

void TestApp::onWindowResize() {
	Super::onWindowResize();
	_camera.init(dimension());
	_camera.setAspectRatio(_aspect);
}

core::AppState TestApp::onInit() {
	core::Var::get(cfg::ClientFullscreen, "false");
	core::Var::get(cfg::ClientWindowWidth, "640");
	core::Var::get(cfg::ClientWindowHeight, "480");

	const core::AppState state = Super::onInit();
	_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!_plane.init(glm::zero<glm::vec3>())) {
		return core::AppState::Cleanup;
	}

	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _dimension.x, _dimension.y, _aspect);
	_camera.init(dimension());
	_camera.setAspectRatio(_aspect);
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	core::Command::registerCommand("+cam_freelook", [&] (const core::CmdArgs& args) {
		Log::info("target lock: %s", args[0].c_str());
		if (args[0] == "true") {
			_camera.setRotationType(video::CameraRotationType::Target);
			_camera.setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
			return;
		}
		_camera.setRotationType(video::CameraRotationType::Eye);
	}).setHelp("Camera free look on toggle");

	const glm::vec4& color = ::core::Color::Black;
	glClearColor(color.r, color.g, color.b, color.a);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return state;
}

void TestApp::beforeUI() {
	Super::beforeUI();

	if (_cameraMotion) {
		SDL_WarpMouseInWindow(_window, width() / 2, height() / 2);
	}

	const float speed = _cameraSpeed * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);

	_camera.update(_deltaFrame);

	if  (_renderPlane) {
		_plane.render(_camera);
	}
	doRender();
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

core::AppState TestApp::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
	return Super::onCleanup();
}

void TestApp::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
}

void TestApp::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	if (_cameraMotion) {
		_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
	}
}
