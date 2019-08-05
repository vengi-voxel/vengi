/**
 * @file
 */

#include "TestApp.h"
#include "ui/imgui/IMGUI.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "video/ScopedPolygonMode.h"

TestApp::TestApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "test");
}

TestApp::~TestApp() {
}

void TestApp::onWindowResize(int windowWidth, int windowHeight) {
	Super::onWindowResize(windowWidth, windowHeight);
	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
}

core::AppState TestApp::onConstruct() {
	core::AppState state = Super::onConstruct();
	core::Var::get(cfg::ClientFullscreen, "false");
	core::Var::get(cfg::ClientWindowWidth, "1024");
	core::Var::get(cfg::ClientWindowHeight, "768");

	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	_movement.construct();

	core::Command::registerCommand("+cam_freelook", [&] (const core::CmdArgs& args) {
		Log::info("target lock: %s", args[0].c_str());
		if (args[0] == "true") {
			_camera.setRotationType(video::CameraRotationType::Target);
			_camera.setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
			return;
		}
		_camera.setRotationType(video::CameraRotationType::Eye);
	}).setHelp("Camera free look on toggle");

	core::Command::registerCommand("togglerelativemouse", [&] (const core::CmdArgs& args) {
		_cameraMotion ^= true;
	}).setHelp("Toggle relative mouse rotation mode");

	return state;
}

core::AppState TestApp::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	_logLevelVar->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_axis.init()) {
		return core::AppState::InitFailure;
	}

	if (!_plane.init() || !_plane.plane(glm::zero<glm::vec3>(), 0, _planeColor)) {
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		return core::AppState::InitFailure;
	}

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _frameBufferDimension.x, _frameBufferDimension.y, _aspect);
	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0f));

	video::clearColor(::core::Color::Black);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	return state;
}

void TestApp::beforeUI() {
	Super::beforeUI();
	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			_camera.rotate(glm::vec3(_mouseRelativePos.y,_mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
			SDL_WarpMouseInWindow(_window, frameBufferWidth() / 2, frameBufferHeight() / 2);
		}
	}

	_movement.update(_deltaFrameMillis);
	const glm::vec3& moveDelta = _movement.moveDelta(_cameraSpeed);
	_camera.move(moveDelta);
	_camera.update(_deltaFrameMillis);

	if (_renderPlane) {
		_plane.render(_camera, glm::scale(glm::vec3(100.0f)));
	}
	{
		core_trace_scoped(TestAppDoRender);
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		doRender();
	}
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

core::AppState TestApp::onRunning() {
	const core::AppState state = Super::onRunning();
	SDL_SetRelativeMouseMode(_cameraMotion ? SDL_TRUE : SDL_FALSE);
	return state;
}

void TestApp::onRenderUI() {
	ImGui::BulletText("ESC: toggle camera free look");
	bool temp = _renderTracing;
	if (ImGui::Checkbox("Toggle profiler", &temp)) {
		_renderTracing = toggleTrace();
	}
	ImGui::Checkbox("Render axis", &_renderAxis);
	ImGui::Checkbox("Render plane", &_renderPlane);
	ImGui::Checkbox("Camera motion", &_cameraMotion);
	ImGui::InputFloat("Camera speed", &_cameraSpeed, 0.02f, 0.1f);
	glm::vec3 cameraPos = _camera.position();
	if (ImGui::InputFloat3("Camera position", glm::value_ptr(cameraPos))) {
		_camera.setPosition(cameraPos);
	}
	ImGui::InputVarFloat("Rotation speed", _rotationSpeed, 0.01f, 0.1f);
	ImGui::Separator();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
}

core::AppState TestApp::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	_movement.shutdown();
	return Super::onCleanup();
}

bool TestApp::onMouseWheel(int32_t x, int32_t y) {
	const bool retVal = Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
	return retVal;
}
