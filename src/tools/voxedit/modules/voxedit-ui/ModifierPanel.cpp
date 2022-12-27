/**
 * @file
 */

#include "ModifierPanel.h"
#include "IMGUIApp.h"
#include "ScopedStyle.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "ui/Toolbar.h"
#include "voxedit-ui/Util.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

void ModifierPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.setFont(imguiApp()->bigIconFont());
	const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
	ui::Toolbar toolbar(buttonSize);
	toolbar.button(ICON_FA_CUBE, "actionplace");
	toolbar.button(ICON_FA_ERASER, "actionerase");
	toolbar.button(ICON_FA_DIAGRAM_NEXT, "actionoverride");
	toolbar.button(ICON_FA_PAINTBRUSH, "actionpaint");
	toolbar.button(ICON_FA_EXPAND, "actionselect");
	toolbar.button(ICON_FA_ELLIPSIS, "actionpath");
	toolbar.button(ICON_FA_ELLIPSIS, "actionline");
	toolbar.button(ICON_FA_EYE_DROPPER, "actioncolorpicker");
}

bool ModifierPanel::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	ui::ScopedStyle style;
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

	ui::ScopedStyle style;
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
	Modifier &modifier = sceneMgr().modifier();
	const bool plane = modifier.planeMode();

	ui::ScopedStyle style;
	// mirror planes are disabled in plane mode
	if (plane) {
		style.disableItem();
		modifier.setMirrorAxis(math::Axis::None, glm::ivec3(0));
	}
	mirrorAxisRadioButton("Disable mirror##mirror", math::Axis::None);
	ImGui::SameLine();
	mirrorAxisRadioButton("X##mirror", math::Axis::X);
	ImGui::TooltipText("Mirror along the x axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Y##mirror", math::Axis::Y);
	ImGui::TooltipText("Mirror along the y axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Z##mirror", math::Axis::Z);
	ImGui::TooltipText("Mirror along the z axis at the reference position");
}

void ModifierPanel::addModifierModes() {
	Modifier &modifier = sceneMgr().modifier();
	bool plane = modifier.planeMode();
	if (ImGui::Checkbox("Plane##modifiertype", &plane)) {
		modifier.setPlaneMode(plane);
	}
	ImGui::TooltipText("Modifies the whole plane or connected voxels - can be seen as extrude feature");
	bool single = modifier.singleMode();
	if (ImGui::Checkbox("Single##modifiertype", &single)) {
		modifier.setSingleMode(single);
	}
	ImGui::TooltipText("Only interact with single voxels - don't span an area - one click one modification");
	bool center = modifier.centerMode();
	if (ImGui::Checkbox("Center##modifiertype", &center)) {
		modifier.setCenterMode(center);
	}
	ImGui::TooltipText("This is using the point of the click to span the area - not one of the edges");
}

void ModifierPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(ToolsPanel);
		addModifiers(listener);
		ImGui::Separator();
		addModifierModes();
		addShapes();
		addMirrorPlanes();
	}
	ImGui::End();
}

} // namespace voxedit
