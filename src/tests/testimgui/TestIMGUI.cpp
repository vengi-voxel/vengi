/**
 * @file
 */
#include "TestIMGUI.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "testcore/TestAppMain.h"
#include "ui/IMGUIEx.h"
#include "ui/IMGUIStyle.h"
#include "ui/IconsLucide.h"
#include "engine-config.h"
#ifdef USE_IMPLOT_DEMO
#include "ui/dearimgui/implot.h"
#endif
#include "video/Renderer.h"

TestIMGUI::TestIMGUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "testimgui");
	setRenderAxis(false);
	setCameraMotion(false);
	_fullScreenApplication = false;
	_windowWidth = 1280;
	_windowHeight = 800;
}

app::AppState TestIMGUI::onConstruct() {
	const app::AppState state = Super::onConstruct();
	// Keep widgets in the main framebuffer so --screenshot captures the UI.
	core::Var::getVar(cfg::UIMultiMonitor)->setVal("false");
	return state;
}

void TestIMGUI::renderStylePreview() {
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(display.x - 24.0f, display.y - 24.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Style Preview", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("New");
			ImGui::MenuItem("Open");
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Demo Window", nullptr, &_showTestWindow);
			ImGui::MenuItem("Metrics", nullptr, &_showMetricsWindow);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Headline("Widget spacing");
	ImGui::TextWrappedUnformatted("Typical vengi widgets used to judge padding, item spacing and color themes.");
	const core::VarPtr &uiStyleVar = core::getVar(cfg::UIStyle);
	const int currentStyle = uiStyleVar->intVal();
	if (ImGui::BeginCombo("Color theme", ImGui::GetStyleName(currentStyle))) {
		for (int i = 0; i < ImGui::MaxStyles; ++i) {
			const bool isSelected = i == currentStyle;
			if (ImGui::Selectable(ImGui::GetStyleName(i), isSelected)) {
				uiStyleVar->setVal(i);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator();

	if (ImGui::BeginTable("##stylepreview_cols", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV)) {
		ImGui::TableNextColumn();

		ImGui::TextUnformatted("Buttons");
		ImGui::Button("Apply");
		ImGui::SameLine();
		ImGui::Button("Reset");
		ImGui::SameLine();
		ImGui::DisabledButton("Disabled", true);
		ImGui::IconButton(ICON_LC_PLUS, "Add");
		ImGui::SameLine();
		ImGui::IconButton(ICON_LC_TRASH, "Delete");
		ImGui::SameLine();
		ImGui::ToggleButton("Toggle", _checkbox);

		ImGui::Separator();
		ImGui::Checkbox("Grid visible", &_checkbox);
		ImGui::SameLine();
		ImGui::BeginDisabled();
		ImGui::Checkbox("Locked", &_checkboxDisabled);
		ImGui::EndDisabled();
		ImGui::RadioButton("Brush", &_radio, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Select", &_radio, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Erase", &_radio, 2);

		ImGui::Separator();
		ImGui::SliderFloat("Intensity", &_slider, 0.0f, 1.0f, "%.2f");
		ImGui::SliderInt("Size", &_sliderInt, 1, 16);
		ImGui::ProgressBar(_progress, ImVec2(-1.0f, 0.0f));

		ImGui::TableNextColumn();

		ImGui::InputText("Name", &_input);
		const char *brushItems[] = {"Box", "Sphere", "Plane", "Path"};
		ImGui::Combo("Brush type", &_combo, brushItems, IM_ARRAYSIZE(brushItems));
		ImGui::InputXYZ("Position", _position);
		ImGui::ColorEdit4("Accent", &_color.x, ImGuiColorEditFlags_NoInputs);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextMultiline("##notes", &_input, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4.0f));

		if (ImGui::CollapsingHeader("Advanced", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BulletText("Window, frame and item padding");
			ImGui::BulletText("Table cell padding");
			ImGui::BulletText("Tree indent spacing");
		}

		ImGui::EndTable();
	}

	if (ImGui::BeginTabBar("##stylepreview_tabs")) {
		if (ImGui::BeginTabItem("Table")) {
			if (ImGui::BeginTable("##stylepreview_table", 3,
								  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Layer");
				ImGui::TableSetupColumn("Voxels");
				ImGui::TableSetupColumn("Visible");
				ImGui::TableHeadersRow();
				for (int i = 0; i < 4; ++i) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Layer %d", i + 1);
					ImGui::TableNextColumn();
					ImGui::Text("%d", (i + 1) * 128);
					ImGui::TableNextColumn();
					ImGui::Checkbox(("##vis" + core::string::toString(i)).c_str(), &_checkbox);
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Tree")) {
			if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
					ImGui::TreeNodeEx("Pivot", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
					ImGui::TreePop();
				}
				ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
				ImGui::TreePop();
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Child")) {
			if (ImGui::BeginChild("##stylepreview_child", ImVec2(0.0f, ImGui::Height(8)), ImGuiChildFlags_Borders)) {
				for (int i = 0; i < 8; ++i) {
					ImGui::Selectable(("Node " + core::string::toString(i)).c_str(), i == _listbox);
				}
			}
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Separator();
	if (ImGui::Button("Demo Window")) {
		_showTestWindow ^= true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Metrics")) {
		_showMetricsWindow ^= true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Implot")) {
		_showImPlotWindow ^= true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
	ImGui::SameLine();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
				ImGui::GetIO().Framerate);

	ImGui::End();
}

void TestIMGUI::onRenderUI() {
	renderStylePreview();

	if (_showMetricsWindow) {
		ImGui::ShowMetricsWindow(&_showMetricsWindow);
	}
	if (_showImPlotWindow) {
#if USE_IMPLOT_DEMO
		ImPlot::ShowDemoWindow();
#endif
	}

	if (_showTestWindow) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(ImGui::Size(40), ImGui::Height(20)), ImGuiCond_FirstUseEver);
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

	video::clearColor(::color::Black());
	return state;
}

void TestIMGUI::doRender() {
}

TEST_APP(TestIMGUI)
