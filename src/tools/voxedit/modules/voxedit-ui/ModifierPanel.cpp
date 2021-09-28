/**
 * @file
 */

#include "ModifierPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"

namespace voxedit {

bool ModifierPanel::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	if (ImGui::RadioButton(title, modifier.mirrorAxis() == type)) {
		modifier.setMirrorAxis(type, sceneMgr().referencePosition());
		return true;
	}
	return false;
}

void ModifierPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		const float windowWidth = ImGui::GetWindowWidth();
		ImGui::CommandButton(ICON_FA_CROP " Crop layer", "crop", "Crop the current layer to the voxel boundaries", windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_EXPAND_ARROWS_ALT " Extend all layers", "resize", nullptr, windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_OBJECT_UNGROUP " Layer from color", "colortolayer", "Create a new layer from the current selected color", windowWidth, &listener);
		ImGui::CommandButton(ICON_FA_COMPRESS_ALT " Scale", "scale", "Scale the current layer down", windowWidth, &listener);

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::CommandButton(ICON_FK_REPEAT " X", "rotate 90 0 0", nullptr, windowWidth / 3.0f, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the x axis");
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FK_REPEAT " Y", "rotate 0 90 0", nullptr, windowWidth / 3.0f, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the y axis");
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FK_REPEAT " Z", "rotate 0 0 90", nullptr, windowWidth / 3.0f, &listener);
			ImGui::TooltipText("Rotate by 90 degree on the z axis");
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::CommandButton("X", "flip x", nullptr, windowWidth / 3.0f, &listener);
			ImGui::SameLine();
			ImGui::CommandButton("Y", "flip y", nullptr, windowWidth / 3.0f, &listener);
			ImGui::SameLine();
			ImGui::CommandButton("Z", "flip z", nullptr, windowWidth / 3.0f, &listener);
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("none", math::Axis::None);
			mirrorAxisRadioButton("x", math::Axis::X);
			mirrorAxisRadioButton("y", math::Axis::Y);
			mirrorAxisRadioButton("z", math::Axis::Z);
		}
	}
	ImGui::End();
}

}
