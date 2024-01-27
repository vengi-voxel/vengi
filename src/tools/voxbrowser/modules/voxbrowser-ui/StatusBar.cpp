/**
 * @file
 */

#include "StatusBar.h"
#include "ui/ScopedStyle.h"

namespace voxbrowser {

void StatusBar::update(const char *title, float height) {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(viewport->ID);
	const ImVec2 &size = viewport->WorkSize;
	ImGui::SetNextWindowSize(ImVec2(size.x, height));
	ImVec2 statusBarPos = viewport->WorkPos;
	statusBarPos.y += size.y - height;
	ImGui::SetNextWindowPos(statusBarPos);
	const uint32_t statusBarFlags =
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin(title, nullptr, statusBarFlags)) {
		ui::ScopedStyle scopedStyle;
		scopedStyle.setItemSpacing(ImVec2(20, 0));
		ImGui::Text("x entries");
	}
	ImGui::End();
}

} // namespace voxbrowser
