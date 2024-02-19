/**
 * @file
 */

#include "LSystemPanel.h"
#include "IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IMGUIEx.h"

namespace voxedit {

bool LSystemPanel::init() {
	// TODO: load lsystem settings
	return true;
}

void LSystemPanel::update(const char *title) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(LSystemPanel);
		ImGui::InputText(_("Axiom##noise"), &_lsystemData.axiom);
		ImGui::InputTextMultiline(_("Rules##noise"), &_lsystemData.rulesStr);
		ImGui::InputFloat(_("Angle##noise"), &_lsystemData.angle);
		ImGui::InputFloat(_("Length##noise"), &_lsystemData.length);
		ImGui::InputFloat(_("Width##noise"), &_lsystemData.width);
		ImGui::InputFloat(_("Width Increment##noise"), &_lsystemData.widthIncrement);
		ImGui::InputInt(_("Iterations##noise"), &_lsystemData.iterations);
		ImGui::InputFloat(_("Leaves Radius##noise"), &_lsystemData.leavesRadius);

		if (ImGui::IconButton(ICON_LC_PLAY, _("OK##lsystem"))) {
			core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
			if (voxelgenerator::lsystem::parseRules(_lsystemData.rulesStr, rules)) {
				sceneMgr().lsystem(_lsystemData.axiom.c_str(), rules, _lsystemData.angle,
					_lsystemData.length, _lsystemData.width, _lsystemData.widthIncrement, _lsystemData.iterations, _lsystemData.leavesRadius);
			}
		}

		const uint32_t tableFlags = ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::GetTextLineHeightWithSpacing() * 6);
		if (ImGui::BeginTable("##lsystemrules", 2, tableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Command##lsystemrules"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description##lsystemrules"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();
			#define ROWENTRY(C, D) ImGui::TableNextColumn(); ImGui::TextUnformatted(C); ImGui::TableNextColumn(); ImGui::TextUnformatted(D)
			ROWENTRY("F", _("Draw line forwards"));
			ROWENTRY("(", _("Set voxel type"));
			ROWENTRY("b", _("Move backwards (no drawing)"));
			ROWENTRY("L", _("Leaf"));
			ROWENTRY("+", _("Rotate right"));
			ROWENTRY("-", _("Rotate left"));
			ROWENTRY(">", _("Rotate forward"));
			ROWENTRY("<", _("Rotate back"));
			ROWENTRY("#", _("Increment width"));
			ROWENTRY("!", _("Decrement width"));
			ROWENTRY("[", _("Push"));
			ROWENTRY("]", _("Pop"));
			#undef ROWENTRY
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void LSystemPanel::shutdown() {
	// TODO: persist the lsystem settings
}

}
