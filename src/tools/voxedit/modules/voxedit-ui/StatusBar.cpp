/**
 * @file
 */

#include "StatusBar.h"
#include "ScopedStyle.h"
#include "core/StringUtil.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/Console.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IMGUIApp.h"

namespace voxedit {

static bool xyzValues(const char *title, glm::ivec3 &v) {
	ImGui::TextUnformatted(title);
	ImGui::SameLine();

	bool retVal = false;
	const float width = ImGui::CalcTextSize("10000").x + ImGui::GetStyle().FramePadding.x * 2.0f;

	char buf[64];
	core::String id = "##";
	id.append(title);
	id.append("0");

	id.c_str()[id.size() - 1] = '0';
	core::string::formatBuf(buf, sizeof(buf), "%i", v.x);
	ui::imgui::ScopedStyle style;
	style.setColor(ImGuiCol_Text, core::Color::Red);
	ImGui::PushItemWidth(width);
	if (ImGui::InputText(id.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
		retVal = true;
		v.x = core::string::toInt(buf);
	}
	ImGui::SameLine(0.0f, 2.0f);

	id.c_str()[id.size() - 1] = '1';
	core::string::formatBuf(buf, sizeof(buf), "%i", v.y);
	style.setColor(ImGuiCol_Text, core::Color::Green);
	ImGui::PushItemWidth(width);
	if (ImGui::InputText(id.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
		retVal = true;
		v.y = core::string::toInt(buf);
	}
	ImGui::SameLine(0.0f, 2.0f);

	id.c_str()[id.size() - 1] = '2';
	core::string::formatBuf(buf, sizeof(buf), "%i", v.z);
	style.setColor(ImGuiCol_Text, core::Color::Blue);
	ImGui::PushItemWidth(width);
	if (ImGui::InputText(id.c_str(), buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
		retVal = true;
		v.z = core::string::toInt(buf);
	}
	ImGui::SameLine(0.0f, 2.0f);

	return retVal;
}

void StatusBar::update(const char *title, float height, const core::String &lastExecutedCommand) {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(viewport->ID);
	const ImVec2 &size = viewport->WorkSize;
	ImGui::SetNextWindowSize(ImVec2(size.x, height));
	ImVec2 statusBarPos = viewport->WorkPos;
	statusBarPos.y += size.y - height;
	ImGui::SetNextWindowPos(statusBarPos);
	const uint32_t statusBarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin(title, nullptr, statusBarFlags)) {
		core_trace_scoped(StatusBar);
		voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		const voxel::SceneGraph& sceneGraph = sceneMgr.sceneGraph();
		const voxedit::ModifierFacade& modifier = sceneMgr.modifier();
		const float fields = 4.0f;
		const int nodeId = sceneGraph.activeNode();
		const voxel::RawVolume* v = sceneMgr.volume(nodeId);
		const voxel::Region& region = v->region();
		glm::ivec3 mins = region.getLowerCorner();
		glm::ivec3 maxs = region.getDimensionsInVoxels();
		if (xyzValues("pos", mins)) {
			const glm::ivec3 &f = mins - region.getLowerCorner();
			sceneMgr.shift(nodeId, f);
		}
		ImGui::SameLine();
		if (xyzValues("size", maxs)) {
			const glm::ivec3 &f = maxs - region.getDimensionsInVoxels();
			sceneMgr.resize(nodeId, f);
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(size.x / fields * 1.0f);
		if (modifier.aabbMode()) {
			const glm::ivec3& dim = modifier.aabbDim();
			ImGui::Text("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
		} else {
			ImGui::TextUnformatted("w: 0, h: 0, d: 0");
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(size.x / fields * 2.0f);
		if (lastExecutedCommand.empty()) {
			ImGui::TextUnformatted("Command: -");
		} else {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			const core::String& keybindingStr = app->getKeyBindingsString(lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				ImGui::Text("Command: %s", lastExecutedCommand.c_str());
			} else {
				ImGui::Text("Command: %s (%s)", lastExecutedCommand.c_str(), keybindingStr.c_str());
			}
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(size.x / fields * 3.0f);
		ImGui::SetNextItemWidth(140.0f);
		ImGui::InputVarInt("Grid size", cfg::VoxEditGridsize);
		ImGui::SameLine();
		ImGui::CheckboxVar("Grayscale", cfg::VoxEditGrayInactive);
		ImGui::SameLine();
		ImGui::CheckboxVar("Only active", cfg::VoxEditHideInactive);
		ImGui::SameLine();
		const EditMode editMode = sceneMgr.editMode();
		if (editMode == EditMode::Animation) {
			ImGui::BeginDisabled();
		}
		bool sceneView = editMode == EditMode::Scene;
		if (ImGui::Checkbox("Scene Mode", &sceneView)) {
			if (sceneView) {
				sceneMgr.setEditMode(EditMode::Scene);
			} else {
				sceneMgr.setEditMode(EditMode::Model);
			}
		}
		if (editMode == EditMode::Animation) {
			ImGui::EndDisabled();
		}
	}
	ImGui::End();
}


}
