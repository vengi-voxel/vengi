#include "TestApp.h"
#include "video/Debug.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "frontend/Movement.h"
#include "ui/TestAppWindow.h"
#include "video/ScopedPolygonMode.h"

TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider, 21000) {
	init("engine", "test");
}

TestApp::~TestApp() {
}

void TestApp::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(), dimension());
}

core::AppState TestApp::onConstruct() {
	core::Var::get(cfg::ClientFullscreen, "false");
	core::Var::get(cfg::ClientWindowWidth, "1024");
	core::Var::get(cfg::ClientWindowHeight, "768");

	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

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

	return Super::onConstruct();
}

core::AppState TestApp::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!_plane.init() || !_plane.plane(glm::zero<glm::vec3>())) {
		return core::AppState::Cleanup;
	}

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _dimension.x, _dimension.y, _aspect);
	_camera.init(glm::ivec2(), dimension());
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));

	video::clearColor(::core::Color::Black);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	new testcore::TestAppWindow(this);

	return state;
}

void TestApp::beforeUI() {
	Super::beforeUI();

	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			SDL_WarpMouseInWindow(_window, width() / 2, height() / 2);
		}
	}

	const float speed = _cameraSpeed * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(_deltaFrame);

	if  (_renderPlane) {
		_plane.render(_camera);
	}
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		doRender();
	}
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

void TestApp::afterRootWidget() {
	Super::afterRootWidget();
	enqueueShowStr(5, core::Color::Gray, "ESC: toggle camera free look");
}

core::AppState TestApp::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	return Super::onCleanup();
}

bool TestApp::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		const SDL_bool current = SDL_GetRelativeMouseMode();
		const SDL_bool mode = current ? SDL_FALSE : SDL_TRUE;
		SDL_SetRelativeMouseMode(mode);
		if (mode) {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_INVISIBLE);
		} else {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_VISIBLE);
		}
	}
	return Super::onKeyPress(key, modifier);
}

void TestApp::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
}

void TestApp::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (!current) {
			return;
		}
		_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
	}
}
