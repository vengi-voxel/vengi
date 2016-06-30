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
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

void TestApp::onWindowResize() {
	_camera.init(_width, _height);
}

core::AppState TestApp::onInit() {
	const core::AppState state = Super::onInit();

	GLDebug::enable(GLDebug::Medium);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	_camera.init(_width, _height);
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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_camera.setAspectRatio(_aspect);
	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	_camera.update();

	doRender();

	_axis.render(_camera);

	return state;
}

core::AppState TestApp::onCleanup() {
	_axis.shutdown();
	return Super::onCleanup();
}
