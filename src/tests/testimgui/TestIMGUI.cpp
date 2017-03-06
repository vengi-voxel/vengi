#include "io/Filesystem.h"
#include "TestIMGUI.h"
#include "core/Color.h"
#include "imgui_node_graph_test.h"

TestIMGUI::TestIMGUI(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
}

void TestIMGUI::onRenderUI() {
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::ColorEdit3("clear color", (float*) &_clearColor);
		if (ImGui::Button("Test Window")) {
			_showTestWindow ^= true;
			Log::info("button x pressed: %i", (int) _showTestWindow);
		}
		if (ImGui::Button("Another Window")) {
			_showAnotherWindow ^= true;
			Log::info("button y pressed: %i", (int) _showAnotherWindow);
		}
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		if (ImGui::Button("Quit")) {
			requestQuit();
		}
	}

	ShowExampleAppCustomNodeGraph(&_showNodeGraphWindow);

	// 2. Show another simple window, this time using an explicit Begin/End pair
	if (_showAnotherWindow) {
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Another Window", &_showAnotherWindow);
		ImGui::Text("Hello");
		ImGui::End();
	}
	if (_showTestWindow) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&_showTestWindow);
	}
}

core::AppState TestIMGUI::onInit() {
	core::AppState state = Super::onInit();
	_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	video::clearColor(::core::Color::Black);
	video::enableDebug(video::DebugSeverity::Medium);
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestIMGUI app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
