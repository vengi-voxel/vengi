#include "TestApp.h"
#include "core/AppModule.h"
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
	_camera.init(_width, _height);
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

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _width, _height, _aspect);
	_camera.init(_width, _height);
	_camera.setAspectRatio(_aspect);
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.lookAt(glm::vec3(0.0f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

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

core::AppState TestApp::onRunning() {
	const core::AppState state = Super::onRunning();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	if (left || right || forward || backward) {
		const glm::vec3& pos = _camera.position();
		Log::info("camera: %f:%f:%f", pos.x, pos.y, pos.z);
	}
	_camera.update();

	doRender();

	_axis.render(_camera);

	return state;
}

core::AppState TestApp::onCleanup() {
	_axis.shutdown();
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
	return Super::onCleanup();
}
