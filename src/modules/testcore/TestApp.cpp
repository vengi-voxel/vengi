/**
 * @file
 */

#include "TestApp.h"
#include "IMGUIEx.h"
#include "app/i18n/Language.h"
#include "color/Color.h"
#include "command/Command.h"
#include "video/ScopedPolygonMode.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "core/Log.h"
#include <SDL_hints.h>
#include <SDL_version.h>
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

TestApp::TestApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "test");
	_systemLanguage = app::Language::fromSpec("aa");
}

TestApp::~TestApp() {
}

void TestApp::onWindowResize(void *windowHandle, int windowWidth, int windowHeight) {
	Super::onWindowResize(windowHandle, windowWidth, windowHeight);
	camera().setSize(windowDimension());
}

app::AppState TestApp::onConstruct() {
	app::AppState state = Super::onConstruct();

	_rotationSpeed = core::Var::getVar(cfg::ClientMouseRotationSpeed);

	_movement.construct();

	command::Command::registerCommand("+cam_freelook")
		.addArg({"enabled", command::ArgType::String, true, "true", "Enable or disable target lock"})
		.setHandler([this] (const command::CommandArgs& args) {
			const core::String &enabled = args.str("enabled", "true");
			Log::info("target lock: %s", enabled.c_str());
			if (enabled == "true") {
				camera().setRotationType(video::CameraRotationType::Target);
				camera().setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
				return;
			}
			camera().setRotationType(video::CameraRotationType::Eye);
		}).setHelp(_("Camera free look on toggle"));

	command::Command::registerCommand("togglerelativemouse")
		.setHandler([&] (const command::CommandArgs& args) {
			_cameraMotion ^= true;
		}).setHelp(_("Toggle relative mouse rotation mode"));

	return state;
}

app::AppState TestApp::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
#if SDL_VERSION_ATLEAST(2, 30, 0)
	SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");
#endif
	_logLevelVar->setVal((int)Log::Level::Debug);
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

	video::clearColor(::color::Black());
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	video::enable(video::State::Blend);

	command::Command::execute("bindlist");

	return state;
}

void TestApp::beforeUI() {
	Super::beforeUI();
	if (_cameraMotion) {
		const bool current = isRelativeMouseMode();
		if (current) {
			camera().rotate(glm::vec3(_mouseRelativePos.y,_mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
			centerMousePosition();
		}
	}

	_movement.update(_nowSeconds);
	const glm::vec3& moveDelta = _movement.moveDelta(_cameraSpeed);
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
	if (ImGui::InputVec3("Camera position", cameraPos)) {
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

bool TestApp::onMouseWheel(void *windowHandle, float x, float y, int32_t mouseId) {
	const bool retVal = Super::onMouseWheel(windowHandle, x, y, mouseId);
	const float targetDistance = glm::clamp(camera().targetDistance() - y, 0.0f, 500.0f);
	camera().setTargetDistance(targetDistance);
	return retVal;
}
