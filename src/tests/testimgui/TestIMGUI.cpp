/**
 * @file
 */
#include "TestIMGUI.h"
#include "implot.h"
#include "testcore/TestAppMain.h"
#include "color/Color.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "video/RendererInterface.h"

TestIMGUI::TestIMGUI(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testimgui");
}

void TestIMGUI::onRenderUI() {
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::ShowUserGuide();
	//ImGui::Separator();
	//ImGui::ShowStyleEditor(&ImGui::GetStyle());
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

	if (_showMetricsWindow) {
		ImGui::ShowMetricsWindow(&_showMetricsWindow);
	}
	if (_showImPlotWindow) {
		ImPlot::ShowDemoWindow();
	}

	if (_showTestWindow) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(650, 20), ImGuiCond_FirstUseEver);
		ImGui::ShowDemoWindow(&_showTestWindow);
	}
}

app::AppState TestIMGUI::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	_logLevelVar->setVal((int)Log::Level::Debug);
	Log::init();

	video::clearColor(::color::Color::Black());
	//video::enableDebug(video::DebugSeverity::Medium);
	return state;
}

TEST_APP(TestIMGUI)
