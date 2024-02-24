/**
 * @file
 */

#include "StatusBar.h"
#include <glm/common.hpp>
#include "ui/ScopedStyle.h"

namespace voxbrowser {

void StatusBar::update(const char *title, float height, int entries, int allEntries) {
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
		ImGui::Text("%i/%i entries", entries, allEntries);
	}
	if (_downloadActive) {
		ImGui::SameLine();
		ImGui::ProgressBar(_downloadProgress, ImVec2(-1, 0), "Downloading...");
	}
	ImGui::End();
}

void StatusBar::downloadProgress(float value) {
	_downloadProgress = glm::clamp(value, 0.0f, 1.0f);
	_downloadActive = _downloadProgress > 0.0f && _downloadProgress < 1.0f;
}

} // namespace voxbrowser
