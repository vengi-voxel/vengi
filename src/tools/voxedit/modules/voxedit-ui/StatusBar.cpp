/**
 * @file
 */

#include "StatusBar.h"
#include "core/StringUtil.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "ui/ScopedStyle.h"
#include "ui/Console.h"
#include "ui/IMGUIEx.h"
#include "ui/IMGUIApp.h"

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
		ui::ScopedStyle scopedStyle;
		scopedStyle.setItemSpacing(ImVec2(20, 0));
		voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		const voxedit::ModifierFacade& modifier = sceneMgr.modifier();
		ImGui::CheckboxVar("Grayscale", cfg::VoxEditGrayInactive);
		ImGui::SameLine();
		ImGui::CheckboxVar("Only active", cfg::VoxEditHideInactive);
		ImGui::SameLine();

		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputVarInt("Grid size", cfg::VoxEditGridsize);
		ImGui::SameLine();
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
	}
	ImGui::End();
}


}
