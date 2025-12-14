/**
 * @file
 */

#include "LSystemPanel.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include <glm/trigonometric.hpp>
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "voxelgenerator/LSystem.h"

namespace voxedit {

bool LSystemPanel::init() {
	_templates = voxelgenerator::lsystem::defaultTemplates();
	if (!_templates.empty()) {
		_templateIdx = 0;
		_conf = _templates[0].config;
	}
	return true;
}

static int LSystemInputValidator(ImGuiInputTextCallbackData *data) {
	if (core::string::isAlphaNum(data->EventChar)) {
		return 0;
	}
	for (const voxelgenerator::lsystem::LSystemCommand &cmd : voxelgenerator::lsystem::getLSystemCommands()) {
		if (cmd.command == (char)data->EventChar) {
			return 0;
		}
	}
	return 1;
};

void LSystemPanel::update(const char *id) {
	core_trace_scoped(LSystemPanel);
	const core::String title = makeTitle(ICON_LC_LEAF, _("L-System"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu(_("Edit"))) {
				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_COPY,_("Copy"))) {
					copyRulesToClipboard();
				}
				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_PASTE,_("Paste"))) {
					pasteRulesFromClipboard();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
			if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_C)) {
				copyRulesToClipboard();
			}
			if (ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_V)) {
				pasteRulesFromClipboard();
			}
		}

		ImGui::InputText(_("Axiom"), &_conf.axiom);
		ImGui::TooltipTextUnformatted(_("The initial state of the L-System"));

		ImGui::Separator();
		ImGui::TextUnformatted(_("Rules"));
		ImGui::TooltipTextUnformatted(_("The production rules for the L-System"));

		if (ImGui::BeginTable("##rules_editor", 3,
							  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn(_("Predecessor"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Successor"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn(_("Action"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			for (int i = 0; i < (int)_conf.rules.size(); ++i) {
				voxelgenerator::lsystem::Rule &rule = _conf.rules[i];
				ImGui::PushID(i);
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				char buf[2] = {rule.a, '\0'};
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 2);
				if (ImGui::InputText("##a", buf, sizeof(buf))) {
					if (buf[0] != 0) {
						rule.a = buf[0];
					}
				}

				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputText("##b", &rule.b, ImGuiInputTextFlags_CallbackCharFilter, LSystemInputValidator);

				ImGui::TableNextColumn();
				if (ImGui::Button(ICON_LC_TRASH)) {
					_conf.rules.erase(i);
					i--;
				}
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_COPY)) {
					voxelgenerator::lsystem::Rule newRule = rule;
					_conf.rules.insert(_conf.rules.begin() + i + 1, newRule);
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (ImGui::IconButton(ICON_LC_PLUS, _("Add Rule"))) {
			_conf.rules.emplace_back();
		}
		float angle = glm::degrees(_conf.angle);
		if (ImGui::InputFloat(_("Angle"), &angle, 1.0f, 10.0f, "%.1f")) {
			_conf.angle = glm::radians(angle);
		}
		ImGui::TooltipTextUnformatted(_("The angle in degrees"));
		ImGui::InputFloat(_("Length"), &_conf.length);
		ImGui::TooltipTextUnformatted(_("The length of the segments"));
		ImGui::InputFloat(_("Width"), &_conf.width);
		ImGui::TooltipTextUnformatted(_("The initial width of the segments"));
		ImGui::InputFloat(_("Width increment"), &_conf.widthIncrement);
		ImGui::TooltipTextUnformatted(_("The amount to increment/decrement the width"));
		ImGui::InputInt(_("Iterations"), &_conf.iterations);
		ImGui::TooltipTextUnformatted(_("The number of iterations to run"));
		ImGui::InputFloat(_("Leaves radius"), &_conf.leafRadius);
		ImGui::TooltipTextUnformatted(_("The radius of the leaves"));

		if (ImGui::BeginCombo(_("Templates"),
							  _templateIdx >= 0 ? _templates[_templateIdx].name.c_str() : _("Select a template"))) {
			for (int i = 0; i < (int)_templates.size(); ++i) {
				const bool isSelected = (_templateIdx == i);
				if (ImGui::Selectable(_templates[i].name.c_str(), isSelected)) {
					_templateIdx = i;
					_conf = _templates[i].config;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (_templateIdx >= 0 && !_templates[_templateIdx].description.empty()) {
			ImGui::TextWrapped("%s", _templates[_templateIdx].description.c_str());
		}

		if (ImGui::Button(_("Adopt Dimensions"))) {
			const int nodeId = _sceneMgr->sceneGraph().activeNode();
			if (nodeId != InvalidNodeId) {
				const scenegraph::SceneGraphNode &node = _sceneMgr->sceneGraph().node(nodeId);
				if (const voxel::RawVolume *v = _sceneMgr->sceneGraph().resolveVolume(node)) {
					const voxel::Region &region = v->region();
					const glm::ivec3 &mins = region.getLowerCorner();
					const glm::ivec3 dim = region.getDimensionsInVoxels();
					const glm::ivec3 bottomCenter = mins + glm::ivec3(dim.x / 2, 0, dim.z / 2);
					_sceneMgr->modifier().setReferencePosition(bottomCenter);

					const float oldLength = _conf.length;
					_conf.length = (float)dim.y / (float)(_conf.iterations + 1);
					if (_conf.length < 1.0f) {
						_conf.length = 1.0f;
					}
					if (oldLength > 0.0001f) {
						const float scale = _conf.length / oldLength;
						_conf.width *= scale;
						_conf.widthIncrement *= scale;
						_conf.leafRadius *= scale;
					}
				}
			}
		}
		ImGui::TooltipTextUnformatted(_("Adopt the L-System parameters to the current volume dimensions"));

		if (ImGui::OkButton()) {
			_conf.position = _sceneMgr->modifier().referencePosition();
			_sceneMgr->lsystem(_conf);
		}
		if (_sceneMgr->lsystemRunning()) {
			ImGui::SameLine();
			if (ImGui::CancelButton()) {
				_sceneMgr->lsystemAbort();
			}
			ImGui::SameLine();
			ImGui::Spinner("running_lsystem", ImGui::Size(1.0f));
			ImGui::TooltipText(_("Progress: %.1f%%"), _sceneMgr->lsystemProgress() * 100.0f);
		}

		const uint32_t tableFlags = ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::Height(6.0f));
		if (ImGui::BeginTable("##lsystemrules", 2, tableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Command"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();
			for (const voxelgenerator::lsystem::LSystemCommand &cmd : voxelgenerator::lsystem::getLSystemCommands()) {
				ImGui::TableNextColumn();
				char buf[2] = {cmd.command, '\0'};
				ImGui::TextUnformatted(buf);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(_(cmd.description));
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void LSystemPanel::shutdown() {
}

void LSystemPanel::copyRulesToClipboard() {
	core::String str;
	for (const auto &rule : _conf.rules) {
		str += "{\n";
		str += rule.a;
		str += "\n";
		str += rule.b;
		str += "\n}\n";
	}
	ImGui::SetClipboardText(str.c_str());
}

void LSystemPanel::pasteRulesFromClipboard() {
	const char *text = ImGui::GetClipboardText();
	if (text == nullptr) {
		return;
	}
	core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
	if (voxelgenerator::lsystem::parseRules(text, rules)) {
		_conf.rules = rules;
	}
}

} // namespace voxedit
