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

void StatusBar::update(const core::String &lastExecutedCommand) {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 &size = viewport->WorkSize;
	const float statusBarHeight = ImGui::Size((float)((ui::imgui::IMGUIApp*)video::WindowedApp::getInstance())->fontSize() + 16.0f);
	ImGui::SetNextWindowSize(ImVec2(size.x, statusBarHeight));
	ImVec2 statusBarPos = viewport->WorkPos;
	statusBarPos.y += size.y - statusBarHeight;
	ImGui::SetNextWindowPos(statusBarPos);
	const uint32_t statusBarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;
	if (ImGui::Begin("##statusbar", nullptr, statusBarFlags)) {
		const voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		const voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
		const voxedit::ModifierFacade& modifier = sceneMgr.modifier();

		const int layerIdx = layerMgr.activeLayer();
		const voxel::RawVolume* v = sceneMgr.volume(layerIdx);
		const voxel::Region& region = v->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		const core::String& str = core::string::format("%i:%i:%i / %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		ImGui::Text("%s", str.c_str());
		ImGui::SameLine();

		if (modifier.aabbMode()) {
			const glm::ivec3& dim = modifier.aabbDim();
			const core::String& str = core::string::format("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			ImGui::Text("%s", str.c_str());
		} else if (!lastExecutedCommand.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			core::String statusText;
			const core::String& keybindingStr = app->getKeyBindingsString(lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				statusText = core::string::format("Command: %s", lastExecutedCommand.c_str());
			} else {
				statusText = core::string::format("Command: %s (%s)", lastExecutedCommand.c_str(), keybindingStr.c_str());
			}
			ImGui::Text("%s", statusText.c_str());
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::Size(140.0f));
		ImGui::InputVarInt("Grid size", cfg::VoxEditGridsize);
	}
	ImGui::End();
}


}
