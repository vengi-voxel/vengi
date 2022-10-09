/**
 * @file
 */

#include "ModifierPanel.h"
#include "IconsFontAwesome5.h"
#include "ScopedStyle.h"
#include "ui/imgui/dearimgui/imgui_internal.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

bool ModifierPanel::modifierRadioButton(const char *title, ModifierType type, const char *tooltip) {
	Modifier &modifier = sceneMgr().modifier();
	ModifierType currentType = modifier.modifierType();
	currentType &= ~(ModifierType::Plane | ModifierType::Single);
	if (ImGui::RadioButton(title, currentType == type)) {
		modifier.setModifierType(type);
		return true;
	}
	if (tooltip != nullptr) {
		ImGui::TooltipText("%s", tooltip);
	}
	return false;
}

void ModifierPanel::update(const char *title) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(ToolsPanel);
		modifierRadioButton(ICON_FA_PAINT_ROLLER " Place", ModifierType::Place, "Place voxels in the volume");
		modifierRadioButton(ICON_FA_ERASER " Erase", ModifierType::Erase, "Erase existing voxels in the volume");
		modifierRadioButton(ICON_FA_FILTER " Override", ModifierType::Place | ModifierType::Erase, "Place voxels and override existing ones");
		modifierRadioButton(ICON_FA_PAINT_BRUSH " Paint", ModifierType::Paint, "Update the color of existing voxels");
		modifierRadioButton(ICON_FA_EXPAND " Select", ModifierType::Select, "Select voxels");
		modifierRadioButton(ICON_FA_ELLIPSIS_H " Path", ModifierType::Path, "Walk from reference to cursor position on solid voxels");
		modifierRadioButton(ICON_FA_ELLIPSIS_H " Line", ModifierType::Line, "Walk from reference to cursor position as a straight line");
		modifierRadioButton(ICON_FA_EYE_DROPPER " Pick color", ModifierType::ColorPicker, "Pick the color from the selected voxel");

		ImGui::Separator();

		Modifier &modifier = sceneMgr().modifier();
		bool plane = modifier.planeMode();
		if (ImGui::Checkbox("Plane", &plane)) {
			modifier.setPlaneMode(plane);
		}
		ImGui::SameLine();

		bool single = modifier.singleMode();
		if (ImGui::Checkbox("Single", &single)) {
			modifier.setSingleMode(single);
		}
		ImGui::SameLine();

		bool center = modifier.centerMode();
		if (ImGui::Checkbox("Center", &center)) {
			modifier.setCenterMode(center);
		}

		// shapes are disabled in plane mode
		if (plane) {
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		const ShapeType currentSelectedShapeType = modifier.shapeType();
		if (ImGui::BeginCombo("Shape", ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)ShapeType::Max; ++i) {
				const ShapeType type = (ShapeType)i;
				const bool selected = type == currentSelectedShapeType;
				if (ImGui::Selectable(ShapeTypeStr[i], selected)) {
					modifier.setShapeType(type);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if (plane) {
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}
	ImGui::End();
}

}
