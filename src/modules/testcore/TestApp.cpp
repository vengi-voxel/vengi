/**
 * @file
 */

#include "TestApp.h"
#include "IMGUIEx.h"
#include "core/Color.h"
#include "command/Command.h"
#include "video/ScopedPolygonMode.h"
#include "core/GameConfig.h"
#include "core/Var.h"
#include "core/Log.h"
#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "test");
}

TestApp::~TestApp() {
}

void TestApp::onWindowResize(void *windowHandle, int windowWidth, int windowHeight) {
	Super::onWindowResize(windowHandle, windowWidth, windowHeight);
	camera().setSize(windowDimension());
}

app::AppState TestApp::onConstruct() {
	app::AppState state = Super::onConstruct();

	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	_movement.construct();

	command::Command::registerCommand("+cam_freelook", [this] (const command::CmdArgs& args) {
		Log::info("target lock: %s", args[0].c_str());
		if (args[0] == "true") {
			camera().setRotationType(video::CameraRotationType::Target);
			camera().setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
			return;
		}
		camera().setRotationType(video::CameraRotationType::Eye);
	}).setHelp(_("Camera free look on toggle"));

	command::Command::registerCommand("togglerelativemouse", [&] (const command::CmdArgs& args) {
		_cameraMotion ^= true;
	}).setHelp(_("Toggle relative mouse rotation mode"));

	return state;
}

app::AppState TestApp::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
#ifdef SDL_HINT_SHUTDOWN_DBUS_ON_QUIT
	SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");
#endif
	_logLevelVar->setVal(core::string::toString(SDL_LOG_PRIORITY_DEBUG));
	Log::init();

	video::enableDebug(video::DebugSeverity::Medium);

	_axis.setSize(10.0f, 10.0f, 10.0f);
	if (!_axis.init()) {
		return app::AppState::InitFailure;
	}

	if (!_plane.init() || !_plane.plane(glm::zero<glm::vec3>(), 0, _planeColor)) {
		return app::AppState::InitFailure;
	}

	if (!_movement.init()) {
		return app::AppState::InitFailure;
	}

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _frameBufferDimension.x, _frameBufferDimension.y, _aspect);
	camera().setSize(windowDimension());
	camera().setWorldPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	camera().lookAt(glm::vec3(0.0f));

	video::clearColor(::core::Color::Black());
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	command::Command::execute("bindlist");

	return state;
}

void TestApp::beforeUI() {
	Super::beforeUI();
	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			camera().rotate(glm::vec3(_mouseRelativePos.y,_mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
			SDL_WarpMouseInWindow(_window, frameBufferWidth() / 2, frameBufferHeight() / 2);
		}
	}

	_movement.update(_nowSeconds);
	const glm::vec3& moveDelta = _movement.moveDelta(_cameraSpeed, 0.0f);
	camera().move(moveDelta);
	camera().update(_deltaFrameSeconds);

	if (_renderPlane) {
		_plane.render(camera(), glm::scale(glm::vec3(100.0f)));
	}
	{
		core_trace_scoped(TestAppDoRender);
		video::ScopedPolygonMode polygonMode(camera().polygonMode());
		doRender();
	}
	if (_renderAxis) {
		_axis.render(camera());
	}
}

app::AppState TestApp::onRunning() {
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	const app::AppState state = Super::onRunning();
	_cameraMotion = setRelativeMouseMode(_cameraMotion);
	return state;
}

void TestApp::onRenderUI() {
	ImGui::BulletText("ESC: toggle camera free look");
	ImGui::Checkbox("Render axis", &_renderAxis);
	ImGui::Checkbox("Render plane", &_renderPlane);
	if (_allowRelativeMouseMode) {
		ImGui::Checkbox("Camera motion", &_cameraMotion);
	}
	ImGui::InputFloat("Camera speed", &_cameraSpeed, 0.02f, 0.1f);
	glm::vec3 cameraPos = camera().worldPosition();
	if (ImGui::InputFloat3("Camera position", glm::value_ptr(cameraPos))) {
		camera().setWorldPosition(cameraPos);
	}
	ImGui::InputVarFloat("Rotation speed", _rotationSpeed, 0.01f, 0.1f);
	ImGui::Separator();
	if (ImGui::Button("Bindings")) {
		showBindingsDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button("Textures")) {
		showTexturesDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Show all commands"))) {
		showCommandDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Show all cvars"))) {
		showCvarDialog();
	}
	ImGui::SameLine();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
}

app::AppState TestApp::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	_movement.shutdown();
	return Super::onCleanup();
}

bool TestApp::onMouseWheel(float x, float y) {
	const bool retVal = Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(camera().targetDistance() - y, 0.0f, 500.0f);
	camera().setTargetDistance(targetDistance);
	return retVal;
}
