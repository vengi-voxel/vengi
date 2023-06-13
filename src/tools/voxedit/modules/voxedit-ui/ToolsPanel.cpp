/**
 * @file
 */

#include "ToolsPanel.h"
#include "Toolbar.h"
#include "Util.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::updateSceneMode(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int activeNode = sceneGraph.activeNode();

	if (scenegraph::SceneGraphNode *node = sceneMgr().sceneGraphNode(activeNode)) {
		const scenegraph::SceneGraphNodeType nodeType = node->type();
		if (ImGui::CollapsingHeader("Action", ImGuiTreeNodeFlags_DefaultOpen)) {
			const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
			ui::ScopedStyle style;
			style.setFramePadding(ImVec2(4));
			ui::Toolbar toolbar(buttonSize, &listener);
			toolbar.button(ICON_FA_COPY, "nodeduplicate");
			if (nodeType == scenegraph::SceneGraphNodeType::Model) {
				toolbar.button(ICON_FA_TRASH, "nodedelete");
				toolbar.button(ICON_FA_COPY, "noderef");
				toolbar.button(ICON_FA_DOWN_LEFT_AND_UP_RIGHT_TO_CENTER, "center_origin");
				toolbar.button(ICON_FA_ARROWS_TO_CIRCLE, "center_referenceposition");
			}
		}
	}
}

void ToolsPanel::updateEditMode(command::CommandExecutionListener &listener) {
	if (ImGui::CollapsingHeader("Action", ImGuiTreeNodeFlags_DefaultOpen)) {
		const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ui::ScopedStyle style;
		style.setFramePadding(ImVec2(4));
		ui::Toolbar toolbar(buttonSize, &listener);
		toolbar.button(ICON_FA_CROP, "crop");
		toolbar.button(ICON_FA_EXPAND, "layersize");
		toolbar.button(ICON_FA_OBJECT_UNGROUP, "colortolayer");
		toolbar.button(ICON_FA_COMPRESS, "scaledown");
		toolbar.button(ICON_FK_ARROWS, "scaleup");
		toolbar.button(ICON_FA_FILL_DRIP, "fillhollow");
		toolbar.button(ICON_FK_ERASER, "hollow");
	}

	const float buttonWidth = (float)imguiApp()->fontSize() * 4;
	if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
		veui::AxisButton(math::Axis::X, "X##rotate", "rotate x", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText("Rotate by 90 degree on the x axis");
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, "Y##rotate", "rotate y", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText("Rotate by 90 degree on the y axis");
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, "Z##rotate", "rotate z", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipText("Rotate by 90 degree on the z axis");
	}

	if (ImGui::CollapsingHeader("Text##text", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::InputText("Text##textinput", &_text.input);

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_FK_ARROWS_V "##textinput", &_text.size)) {
			_text.size = glm::clamp(_text.size, 6, 255);
		}
		ImGui::TooltipText("Font size");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputInt(ICON_FK_ARROWS_H "##textinput", &_text.spacing);
		ImGui::TooltipText("Horizontal spacing");

		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::InputInt(ICON_FK_EXPAND "##textinput", &_text.thickness)) {
			_text.thickness = glm::clamp(_text.thickness, 1, 255);
		}
		ImGui::TooltipText("Thickness");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		ImGui::InputFile("Font##textinput", &_text.font, io::format::fonts(), ImGuiInputTextFlags_ReadOnly);

		if (ImGui::Button("Execute##textinput")) {
			sceneMgr().renderText(_text.input.c_str(), _text.size, _text.thickness, _text.spacing, _text.font.c_str());
		}
	}

	ImGui::NewLine();

	if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
		veui::AxisButton(math::Axis::X, ICON_FK_ARROWS_H " X##flip", "flip x", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, ICON_FK_ARROWS_V " Y##flip", "flip y", nullptr, nullptr, buttonWidth,
						 &listener);
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, ICON_FK_ARROWS_H " Z##flip", "flip z", nullptr, nullptr, buttonWidth,
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
