/**
 * @file
 */

#include "StatusBar.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "ui/ScopedStyle.h"
#include "ui/IMGUIEx.h"

namespace voxedit {

void StatusBar::update(const char *id, float height, const core::String &lastExecutedCommand) {
	core_trace_scoped(StatusBar);
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(viewport->ID);
	const ImVec2 &size = viewport->WorkSize;
	ImGui::SetNextWindowSize(ImVec2(size.x, height));
	ImVec2 statusBarPos = viewport->WorkPos;
	statusBarPos.y += size.y - height;
	ImGui::SetNextWindowPos(statusBarPos);
	const uint32_t statusBarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin(id, nullptr, statusBarFlags)) {
		ui::ScopedStyle scopedStyle;
		scopedStyle.setItemSpacing(ImVec2(ImGui::Size(3.0f), 0));
		ImGui::CheckboxVar(_("Grayscale"), cfg::VoxEditGrayInactive);
		ImGui::SameLine();
		ImGui::CheckboxVar(_("Only active"), cfg::VoxEditHideInactive);
		ImGui::SameLine();

		ImGui::SetNextItemWidth(ImGui::Size(14.0f));
		ImGui::InputVarInt(_("Grid size"), cfg::VoxEditGridsize);
		ImGui::SameLine();
		if (lastExecutedCommand.empty()) {
			ImGui::Text(_("Command: %s"), "-");
		} else {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			const core::String& keybindingStr = app->getKeyBindingsString(lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				ImGui::Text(_("Command: %s"), lastExecutedCommand.c_str());
			} else {
				ImGui::Text(_("Command: %s (%s)"), lastExecutedCommand.c_str(), keybindingStr.c_str());
			}
		}
		ImGui::SameLine();
		ImGui::Text(_("FPS: %.2f"), _app->fps());
	}
	ImGui::End();
}


}
