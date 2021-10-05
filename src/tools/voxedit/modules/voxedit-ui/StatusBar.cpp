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

		const int layerIdx = layerMgr.activeLayer();
		const voxel::RawVolume* v = sceneMgr.volume(layerIdx);
		const voxel::Region& region = v->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		ImGui::Text("%i:%i:%i / %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		ImGui::SameLine();

		if (modifier.aabbMode()) {
			const glm::ivec3& dim = modifier.aabbDim();
			ImGui::Text("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
		} else if (!lastExecutedCommand.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			const core::String& keybindingStr = app->getKeyBindingsString(lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				ImGui::Text("Command: %s", lastExecutedCommand.c_str());
			} else {
				ImGui::Text("Command: %s (%s)", lastExecutedCommand.c_str(), keybindingStr.c_str());
			}
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::Size(140.0f));
		ImGui::InputVarInt("Grid size", cfg::VoxEditGridsize);
	}
	ImGui::End();
}


}
