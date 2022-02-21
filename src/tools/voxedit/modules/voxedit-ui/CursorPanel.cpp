/**
 * @file
 */

#include "CursorPanel.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/Config.h"
#include "ui/imgui/IMGUIEx.h"
#include "Util.h"

namespace voxedit {

void CursorPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(CursorPanel);
		if (ImGui::CollapsingHeader(ICON_FA_ARROWS_ALT " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
			static glm::ivec3 translate {0};
			veui::InputAxisInt(math::Axis::X, "X##translate", &translate.x, 1);
			veui::InputAxisInt(math::Axis::X, "Y##translate", &translate.y, 1);
			veui::InputAxisInt(math::Axis::X, "Z##translate", &translate.z, 1);
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
			math::Axis lockedAxis = sceneMgr().lockedAxis();
			if (veui::CheckboxAxisFlags(math::Axis::X, "X##cursorlock", &lockedAxis)) {
				command::executeCommands("lockx", &listener);
			}
			ImGui::TooltipCommand("lockx");
			ImGui::SameLine();
			const int step = core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
			if (veui::InputAxisInt(math::Axis::X, "##cursorx", &cursorPosition.x, step)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}

			if (veui::CheckboxAxisFlags(math::Axis::Y, "Y##cursorlock", &lockedAxis)) {
				command::executeCommands("locky", &listener);
			}
			ImGui::TooltipCommand("locky");
			ImGui::SameLine();
			if (veui::InputAxisInt(math::Axis::Y, "##cursory", &cursorPosition.y, step)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}

			if (veui::CheckboxAxisFlags(math::Axis::Z, "Z##cursorlock", &lockedAxis)) {
				command::executeCommands("lockz", &listener);
			}
			ImGui::TooltipCommand("lockz");
			ImGui::SameLine();
			if (veui::InputAxisInt(math::Axis::Z, "##cursorz", &cursorPosition.z, step)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
		}
	}
	ImGui::End();
}


}
