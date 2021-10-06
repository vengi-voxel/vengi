/**
 * @file
 */

#include "StatusBar.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/Console.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/IMGUIApp.h"

namespace voxedit {

static void xyzValues(const char *title, const glm::ivec3 &v) {
	ImGui::TextUnformatted(title);
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Red);
	ImGui::Text("%i", v.x);
	ImGui::SameLine(0.0f, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Green);
	ImGui::Text("%i", v.y);
	ImGui::SameLine(0.0f, 2.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Blue);
	ImGui::Text("%i", v.z);
	ImGui::SameLine(0.0f, 2.0f);
	ImGui::PopStyleColor(3);
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
		const voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		const voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
		const voxedit::ModifierFacade& modifier = sceneMgr.modifier();
		const float fields = 4.0f;
		const int layerIdx = layerMgr.activeLayer();
		const voxel::RawVolume* v = sceneMgr.volume(layerIdx);
		const voxel::Region& region = v->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getDimensionsInVoxels();
		xyzValues("pos", mins);
		ImGui::SameLine();
		xyzValues("size", maxs);
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
		ImGui::SetNextItemWidth(ImGui::Size(140.0f));
		ImGui::InputVarInt("Grid size", cfg::VoxEditGridsize);
	}
	ImGui::End();
}


}
