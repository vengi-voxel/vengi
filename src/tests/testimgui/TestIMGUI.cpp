/**
 * @file
 */
#include "TestIMGUI.h"
#include "color/Color.h"
#include "core/Log.h"
#include "testcore/TestAppMain.h"
#include "ui/IMGUIEx.h"
#include "ui/dearimgui/implot.h"
#include "video/Renderer.h"

TestIMGUI::TestIMGUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "testimgui");
}

void TestIMGUI::onRenderUI() {
	ImGui::SetNextWindowPos(ImVec2(ImGui::Size(3), ImGui::Height(3)), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(ImGui::Size(60), ImGui::Height(40)), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Debug")) {
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate);
		ImGui::Separator();
		ImGui::ShowUserGuide();
		// ImGui::Separator();
		// ImGui::ShowStyleEditor(&ImGui::GetStyle());
		ImGui::Separator();
		if (ImGui::Button("Test Window")) {
			_showTestWindow ^= true;
		}
		if (ImGui::Button("Metrics Window")) {
			_showMetricsWindow ^= true;
		}
		if (ImGui::Button("Implot")) {
			_showImPlotWindow ^= true;
		}
		if (ImGui::Button("Quit")) {
			requestQuit();
		}
		ImGui::End();
	}

	if (_showMetricsWindow) {
		ImGui::ShowMetricsWindow(&_showMetricsWindow);
	}
	if (_showImPlotWindow) {
		ImPlot::ShowDemoWindow();
	}

	if (_showTestWindow) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ImGui::Size(40), ImGui::Height(20)), ImGuiCond_FirstUseEver);
		ImGui::ShowDemoWindow(&_showTestWindow);
	}
	ImGui::SetNextWindowPos(ImVec2(ImGui::Size(64), ImGui::Height(3)), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(ImGui::Size(40), ImGui::Height(20)), ImGuiCond_FirstUseEver);
}

app::AppState TestIMGUI::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	_logLevelVar->setVal((int)Log::Level::Debug);
	Log::init();

	video::clearColor(::color::Black());
	// video::enableDebug(video::DebugSeverity::Medium);
	return state;
}

TEST_APP(TestIMGUI)
