/**
 * @file
 */
#include "TestIMGUI.h"
#include "io/Filesystem.h"
#include "core/Color.h"

TestIMGUI::TestIMGUI(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testimgui");
}

void TestIMGUI::onRenderUI() {
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::ShowUserGuide();
	ImGui::Separator();
	ImGui::ShowStyleEditor(&ImGui::GetStyle());
	ImGui::Separator();
	if (ImGui::Button("Test Window")) {
		_showTestWindow ^= true;
	}
	if (ImGui::Button("Metrics Window")) {
		_showMetricsWindow ^= true;
	}
	if (ImGui::Button("Quit")) {
		requestQuit();
	}

	if (_showMetricsWindow) {
		ImGui::ShowMetricsWindow(&_showMetricsWindow);
	}

	if (_showTestWindow) {
		ImGui::SetNextWindowPosCenter(ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowDemoWindow(&_showTestWindow);
	}
}

core::AppState TestIMGUI::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	_logLevelVar->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();

	video::clearColor(::core::Color::Black);
	//video::enableDebug(video::DebugSeverity::Medium);
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestIMGUI app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
