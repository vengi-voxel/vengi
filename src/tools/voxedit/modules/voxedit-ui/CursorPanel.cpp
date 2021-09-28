/**
 * @file
 */

#include "CursorPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

namespace voxedit {

void CursorPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		if (ImGui::CollapsingHeader(ICON_FA_ARROWS_ALT " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
			static glm::vec3 translate {0.0f};
			ImGui::InputFloat("X##translate", &translate.x);
			ImGui::InputFloat("Y##translate", &translate.y);
			ImGui::InputFloat("Z##translate", &translate.z);
			if (ImGui::Button(ICON_FA_BORDER_STYLE " Volumes")) {
				sceneMgr().shift(translate.x, translate.y, translate.z);
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_CUBES " Voxels")) {
				sceneMgr().move(translate.x, translate.y, translate.z);
			}
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader(ICON_FA_CUBE " Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
			glm::ivec3 cursorPosition = sceneMgr().modifier().cursorPosition();
			uint32_t lockedAxis = (uint32_t)sceneMgr().lockedAxis();
			if (ImGui::CheckboxFlags("X##cursorlock", &lockedAxis, (uint32_t)math::Axis::X)) {
				command::executeCommands("lockx", &listener);
			}
			ImGui::TooltipText("Lock the x axis");
			ImGui::SameLine();

			if (ImGui::InputInt("X##cursor", &cursorPosition.x)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Y##cursorlock", &lockedAxis, (uint32_t)math::Axis::Y)) {
				command::executeCommands("locky", &listener);
			}
			ImGui::TooltipText("Lock the y axis");
			ImGui::SameLine();

			if (ImGui::InputInt("Y##cursor", &cursorPosition.y)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Z##cursorlock", &lockedAxis, (uint32_t)math::Axis::Z)) {
				command::executeCommands("lockz", &listener);
			}
			ImGui::TooltipText("Lock the z axis");
			ImGui::SameLine();

			if (ImGui::InputInt("Z##cursor", &cursorPosition.z)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
		}
	}
	ImGui::End();
}


}
