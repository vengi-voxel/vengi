/**
 * @file
 */

#include "ToolsPanel.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool ToolsPanel::modifierRadioButton(const char *title, ModifierType type) {
	if (ImGui::RadioButton(title, sceneMgr().modifier().modifierType() == type)) {
		sceneMgr().modifier().setModifierType(type);
		return true;
	}
	return false;
}

void ToolsPanel::update(const char *title) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(ToolsPanel)
		modifierRadioButton(ICON_FA_PEN " Place", ModifierType::Place);
		modifierRadioButton(ICON_FA_EXPAND " Select", ModifierType::Select);
		modifierRadioButton(ICON_FA_ERASER " Delete", ModifierType::Delete);
		modifierRadioButton(ICON_FA_FILTER " Override", ModifierType::Place | ModifierType::Delete);
		modifierRadioButton(ICON_FA_PAINT_BRUSH " Colorize", ModifierType::Update);

		const ShapeType currentSelectedShapeType = sceneMgr().modifier().shapeType();
		if (ImGui::BeginCombo("Shape", ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)ShapeType::Max; ++i) {
				const ShapeType type = (ShapeType)i;
				const bool selected = type == currentSelectedShapeType;
				if (ImGui::Selectable(ShapeTypeStr[i], selected)) {
					sceneMgr().modifier().setShapeType(type);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
}

}
