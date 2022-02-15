/**
 * @file
 */

#include "ModifierPanel.h"
#include "IMGUIApp.h"
#include "Util.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"

namespace voxedit {

bool ModifierPanel::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	ui::imgui::ScopedStyle style;
	veui::AxisStyleText(style, type, false);
	if (ImGui::RadioButton(title, modifier.mirrorAxis() == type)) {
		modifier.setMirrorAxis(type, sceneMgr().referencePosition());
		return true;
	}
	return false;
}

void ModifierPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(ModifierPanel);
		const float windowWidth = ImGui::GetWindowWidth();
		ImGui::CommandButton(ICON_FA_CROP " Crop layer", "crop", "Crop the current layer to the voxel boundaries", windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_EXPAND_ARROWS_ALT " Extend all layers", "resize", nullptr, windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_OBJECT_UNGROUP " Layer from color", "colortolayer", "Create a new layer from the current selected color", windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_COMPRESS_ALT " Scale", "scale", "Scale the current layer down", windowWidth, &listener);

		ImGui::NewLine();

		const float buttonWidth = (float)imguiApp()->fontSize() * 4;
		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, "X##rotate", "rotate 90 0 0", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the x axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, "Y##rotate", "rotate 0 90 0", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the y axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, "Z##rotate", "rotate 0 0 90", ICON_FK_REPEAT, nullptr, buttonWidth, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the z axis");
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, "X##flip", "flip x", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, "Y##flip", "flip y", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, "Z##flip", "flip z", nullptr, nullptr, buttonWidth, &listener);
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("None##mirror", math::Axis::None);
			mirrorAxisRadioButton("X##mirror", math::Axis::X);
			mirrorAxisRadioButton("Y##mirror", math::Axis::Y);
			mirrorAxisRadioButton("Z##mirror", math::Axis::Z);
		}
	}
	ImGui::End();
}

}
