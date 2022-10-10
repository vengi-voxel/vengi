/**
 * @file
 */

#include "ModifierPanel.h"
#include "IconsFontAwesome5.h"
#include "ScopedStyle.h"
#include "ui/imgui/dearimgui/imgui_internal.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "voxedit-ui/Util.h"
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

void ModifierPanel::addShapes() {
	Modifier &modifier = sceneMgr().modifier();
	const bool plane = modifier.planeMode();

	ui::imgui::ScopedStyle style;
	// shapes are disabled in plane mode
	if (plane) {
		style.disableItem();
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
}

void ModifierPanel::addMirrorPlanes() {
	if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
		Modifier &modifier = sceneMgr().modifier();
		const bool plane = modifier.planeMode();

		ui::imgui::ScopedStyle style;
		// mirror planes are disabled in plane mode
		if (plane) {
			style.disableItem();
			modifier.setMirrorAxis(math::Axis::None, glm::ivec3(0));
		}
		mirrorAxisRadioButton("None##mirror", math::Axis::None);
		ImGui::SameLine();
		mirrorAxisRadioButton("X##mirror", math::Axis::X);
		ImGui::SameLine();
		mirrorAxisRadioButton("Y##mirror", math::Axis::Y);
		ImGui::SameLine();
		mirrorAxisRadioButton("Z##mirror", math::Axis::Z);
	}
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
		{
			bool plane = modifier.planeMode();
			if (ImGui::Checkbox("Plane##modifiertype", &plane)) {
				modifier.setPlaneMode(plane);
			}
		}

		ImGui::SameLine();

		{
			bool single = modifier.singleMode();
			if (ImGui::Checkbox("Single##modifiertype", &single)) {
				modifier.setSingleMode(single);
			}
		}

		ImGui::SameLine();

		{
			bool center = modifier.centerMode();
			if (ImGui::Checkbox("Center##modifiertype", &center)) {
				modifier.setCenterMode(center);
			}
		}
		addShapes();
		addMirrorPlanes();
	}
	ImGui::End();
}

}
