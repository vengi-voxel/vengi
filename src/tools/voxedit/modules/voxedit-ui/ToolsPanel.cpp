/**
 * @file
 */

#include "ToolsPanel.h"
#include "Toolbar.h"
#include "Util.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::updateSceneMode(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNode = sceneGraph.activeNode();

	if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(activeNode)) {
		const scenegraph::SceneGraphNodeType nodeType = node->type();
		if (ImGui::CollapsingHeader(_("Action"), ImGuiTreeNodeFlags_DefaultOpen)) {
			ui::ScopedStyle style;
			style.setFont(_app->bigIconFont());
			const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
			ui::Toolbar toolbar(buttonSize, &listener);
			toolbar.button(ICON_LC_COPY, "nodeduplicate");
			toolbar.button(ICON_LC_TRASH, "nodedelete");
			if (nodeType == scenegraph::SceneGraphNodeType::Model) {
				toolbar.button(ICON_LC_COPY, "modelref");
				toolbar.button(ICON_LC_SHRINK, "center_origin");
				toolbar.button(ICON_LC_SHRINK, "center_referenceposition");
			}
		}
	}
}

void ToolsPanel::updateEditMode(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader(_("Action"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ui::ScopedStyle style;
		style.setFont(_app->bigIconFont());
		const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ui::Toolbar toolbar(buttonSize, &listener);
		toolbar.button(ICON_LC_CROP, "crop");
		toolbar.button(ICON_LC_SCALING, "resizetoselection", _sceneMgr->modifier().selections().empty());
		toolbar.button(ICON_LC_SPLIT, "splitobjects");
		toolbar.button(ICON_LC_EXPAND, "modelsize");
		toolbar.button(ICON_LC_UNGROUP, "colortomodel");
		toolbar.button(ICON_LC_CHEVRON_DOWN_SQUARE, "scaledown");
		toolbar.button(ICON_LC_CHEVRON_UP_SQUARE, "scaleup");
		toolbar.button(ICON_LC_PAINT_BUCKET, "fillhollow");
		toolbar.button(ICON_LC_ERASER, "hollow");
		toolbar.button(ICON_LC_X, "clear");
		toolbar.button(ICON_LC_PAINT_BUCKET, "fill");
	}

	const float buttonWidth = (float)_app->fontSize() * 4;
	if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
		veui::AxisButton(math::Axis::X, "X##rotate", "rotate x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText(_("Rotate by 90 degree on the x axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, "Y##rotate", "rotate y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText(_("Rotate by 90 degree on the y axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, "Z##rotate", "rotate z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText(_("Rotate by 90 degree on the z axis"));
	}

	if (ImGui::CollapsingHeader(_("Text##text"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::InputText(_("Text##textinput"), &_text.input);

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_LC_MOVE_VERTICAL "##textinput", &_text.size)) {
			_text.size = glm::clamp(_text.size, 6, 255);
		}
		ImGui::TooltipText(_("Font size"));
		ImGui::SameLine();

		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputInt(ICON_LC_MOVE_HORIZONTAL "##textinput", &_text.spacing);
		ImGui::TooltipText(_("Horizontal spacing"));

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_LC_EXPAND "##textinput", &_text.thickness)) {
			_text.thickness = glm::clamp(_text.thickness, 1, 255);
		}
		ImGui::TooltipText(_("Thickness"));

		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputFile(_("Font##textinput"), &_text.font, io::format::fonts(), ImGuiInputTextFlags_ReadOnly);

		if (ImGui::Button(_("Execute##textinput"))) {
			_sceneMgr->renderText(_text.input.c_str(), _text.size, _text.thickness, _text.spacing, _text.font.c_str());
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader(_("Flip on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		veui::AxisButton(math::Axis::X, ICON_LC_MOVE_HORIZONTAL " X##flip", "flip x", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, ICON_LC_MOVE_VERTICAL " Y##flip", "flip y", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, ICON_LC_MOVE_DIAGONAL " Z##flip", "flip z", nullptr, nullptr, buttonWidth,
						 &listener);
	}
}

void ToolsPanel::update(const char *title, bool sceneMode, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			updateSceneMode(listener);
		} else {
			updateEditMode(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
