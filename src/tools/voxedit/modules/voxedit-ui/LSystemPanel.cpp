/**
 * @file
 */

#include "LSystemPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

namespace voxedit {

void LSystemPanel::update(const char *title) {
	if (ImGui::Begin(title)) {
		ImGui::InputText("Axiom##noise", &_lsystemData.axiom);
		ImGui::InputTextMultiline("Rules##noise", &_lsystemData.rulesStr);
		ImGui::InputFloat("Angle##noise", &_lsystemData.angle);
		ImGui::InputFloat("Length##noise", &_lsystemData.length);
		ImGui::InputFloat("Width##noise", &_lsystemData.width);
		ImGui::InputFloat("Width Increment##noise", &_lsystemData.widthIncrement);
		ImGui::InputInt("Iterations##noise", &_lsystemData.iterations);
		ImGui::InputFloat("Leaves Radius##noise", &_lsystemData.leavesRadius);

		if (ImGui::Button(ICON_FA_CHECK " OK##lsystem")) {
			core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
			if (voxelgenerator::lsystem::parseRules(_lsystemData.rulesStr, rules)) {
				sceneMgr().lsystem(_lsystemData.axiom.c_str(), rules, _lsystemData.angle,
					_lsystemData.length, _lsystemData.width, _lsystemData.widthIncrement, _lsystemData.iterations, _lsystemData.leavesRadius);
			}
		}

		const uint32_t tableFlags = ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize = ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 6);
		if (ImGui::BeginTable("##lsystemrules", 2, tableFlags, outerSize)) {
			ImGui::TableSetupColumn("Command##lsystemrules", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Description##lsystemrules", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();
			#define ROWENTRY(C, D) ImGui::TableNextColumn(); ImGui::TextUnformatted(C); ImGui::TableNextColumn(); ImGui::TextUnformatted(D)
			ROWENTRY("F", "Draw line forwards");
			ROWENTRY("(", "Set voxel type");
			ROWENTRY("b", "Move backwards (no drawing)");
			ROWENTRY("L", "Leaf");
			ROWENTRY("+", "Rotate right");
			ROWENTRY("-", "Rotate left");
			ROWENTRY(">", "Rotate forward");
			ROWENTRY("<", "Rotate back");
			ROWENTRY("#", "Increment width");
			ROWENTRY("!", "Decrement width");
			ROWENTRY("[", "Push");
			ROWENTRY("]", "Pop");
			#undef ROWENTRY
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

}
