/**
 * @file
 */

#include "ToolsPanel.h"
#include "Toolbar.h"
#include "ui/IMGUIApp.h"
#include "ui/IconsFontAwesome6.h"
#include "Util.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(ToolsPanel);

		if (ImGui::CollapsingHeader("Action", ImGuiTreeNodeFlags_DefaultOpen)) {
			const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
			ui::ScopedStyle style;
			style.setFramePadding(ImVec2(4));
			ui::Toolbar toolbar(buttonSize);
			toolbar.button(ICON_FA_CROP, "crop");
			toolbar.button(ICON_FA_EXPAND, "layersize");
			toolbar.button(ICON_FA_OBJECT_UNGROUP, "colortolayer");
			toolbar.button(ICON_FA_COMPRESS, "scale");
			toolbar.button(ICON_FA_FILL_DRIP, "fillhollow");
		}

		const float buttonWidth = (float)imguiApp()->fontSize() * 4;
		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, "X##rotate", "rotate 90 0 0", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the x axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, "Y##rotate", "rotate 0 90 0", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the y axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, "Z##rotate", "rotate 0 0 90", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the z axis");
		}

		if (ImGui::CollapsingHeader("Text##text", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::InputText("Text##text", &_text.input);

			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::InputInt(ICON_FK_ARROWS_V "##text", &_text.size)) {
				_text.size = glm::clamp(_text.size, 6, 255);
			}
			ImGui::TooltipText("Font size");
			ImGui::SameLine();

			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputInt(ICON_FK_ARROWS_H "##text", &_text.spacing);
			ImGui::TooltipText("Horizontal spacing");

			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::InputInt(ICON_FK_EXPAND "##text", &_text.thickness)) {
				_text.thickness = glm::clamp(_text.thickness, 1, 255);
			}
			ImGui::TooltipText("Thickness");

			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputFile("Font##text", &_text.font, io::format::fonts(), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::Button("Execute##text")) {
				sceneMgr().renderText(_text.input.c_str(), _text.size, _text.thickness, _text.spacing, _text.font.c_str());
			}
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, ICON_FK_ARROWS_H " X##flip", "flip x", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, ICON_FK_ARROWS_V " Y##flip", "flip y", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, ICON_FK_ARROWS_H " Z##flip", "flip z", nullptr, nullptr, buttonWidth, &listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
