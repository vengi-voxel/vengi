/**
 * @file
 */

#include "LSystemPanel.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool LSystemPanel::init() {
	// TODO: load lsystem settings
	return true;
}

void LSystemPanel::update(const char *id) {
	core_trace_scoped(LSystemPanel);
	const core::String title = makeTitle(ICON_LC_LEAF, _("L-System"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(LSystemPanel);
		ImGui::InputText(_("Axiom"), &_lsystemData.axiom);
		ImGui::InputTextMultiline(_("Rules"), &_lsystemData.rulesStr);
		ImGui::InputFloat(_("Angle"), &_lsystemData.angle);
		ImGui::InputFloat(_("Length"), &_lsystemData.length);
		ImGui::InputFloat(_("Width"), &_lsystemData.width);
		ImGui::InputFloat(_("Width increment"), &_lsystemData.widthIncrement);
		ImGui::InputInt(_("Iterations"), &_lsystemData.iterations);
		ImGui::InputFloat(_("Leaves radius"), &_lsystemData.leavesRadius);

		if (ImGui::OkButton()) {
			core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
			if (voxelgenerator::lsystem::parseRules(_lsystemData.rulesStr, rules)) {
				_sceneMgr->lsystem(_lsystemData.axiom.c_str(), rules, _lsystemData.angle, _lsystemData.length,
								   _lsystemData.width, _lsystemData.widthIncrement, _lsystemData.iterations,
								   _lsystemData.leavesRadius);
			}
		}

		const uint32_t tableFlags = ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::Height(6.0f));
		if (ImGui::BeginTable("##lsystemrules", 2, tableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Command"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();
#define ROWENTRY(C, D)                                                                                                 \
	ImGui::TableNextColumn();                                                                                          \
	ImGui::TextUnformatted(C);                                                                                         \
	ImGui::TableNextColumn();                                                                                          \
	ImGui::TextUnformatted(D)
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

} // namespace voxedit
