/**
 * @file
 */

#include "BrushPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "Toolbar.h"
#include "core/Algorithm.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/Util.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxedit-util/Clipboard.h"
#include "voxel/Face.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static constexpr const char *BrushTypeIcons[] = {ICON_LC_BOXES, ICON_LC_GROUP, ICON_LC_STAMP};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

void BrushPanel::addShapes(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();

	const ShapeType currentSelectedShapeType = modifier.shapeBrush().shapeType();
	if (ImGui::BeginCombo("Shape", ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)ShapeType::Max; ++i) {
			const ShapeType type = (ShapeType)i;
			const bool selected = type == currentSelectedShapeType;
			if (ImGui::Selectable(ShapeTypeStr[i], selected)) {
				const core::String &typeStr = core::String::lower(ShapeTypeStr[i]);
				const core::String &cmd = "shape" + typeStr; // shapeaabb, ...
				command::executeCommands(cmd, &listener);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

bool BrushPanel::mirrorAxisRadioButton(const char *title, math::Axis type,
									   command::CommandExecutionListener &listener) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	ui::ScopedStyle style;
	veui::AxisStyleText(style, type, false);
	if (ImGui::RadioButton(title, modifier.shapeBrush().mirrorAxis() == type)) {
		core::String cmd = "mirroraxis"; // mirroraxisx, mirroraxisy, mirroraxisz
		cmd += math::getCharForAxis(type);
		command::executeCommands(cmd, &listener);
		return true;
	}
	return false;
}

void BrushPanel::addMirrorPlanes(command::CommandExecutionListener &listener) {
	mirrorAxisRadioButton("Disable mirror##mirror", math::Axis::None, listener);
	ImGui::SameLine();
	mirrorAxisRadioButton("X##mirror", math::Axis::X, listener);
	ImGui::TooltipText("Mirror along the x axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Y##mirror", math::Axis::Y, listener);
	ImGui::TooltipText("Mirror along the y axis at the reference position");
	ImGui::SameLine();
	mirrorAxisRadioButton("Z##mirror", math::Axis::Z, listener);
	ImGui::TooltipText("Mirror along the z axis at the reference position");
}

void BrushPanel::stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette) {
	Modifier &modifier = sceneMgr().modifier();
	ui::ScopedStyle selectionStyle;
	if (modifier.selections().empty()) {
		selectionStyle.disableItem();
	}
	if (ImGui::Button("Use selection")) {
		const Selections &selections = modifier.selections();
		if (!selections.empty()) {
			core::ScopedPtr<voxel::RawVolume> copy(voxedit::tool::copy(node.volume(), selections));
			if (copy) {
				modifier.stampBrush().setVolume(*copy, palette);
			}
		}
	}
	ImGui::TooltipText("Use the current selection as new stamp");
}

void BrushPanel::stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette, command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	StampBrush &brush = modifier.stampBrush();
	ImGui::InputTextWithHint("Model", "Select a model from the asset panel", &_stamp, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			if (brush.load(filename)) {
				_stamp = filename;
			}
		}
		ImGui::EndDragDropTarget();
	}

	bool center = brush.centerMode();
	if (ImGui::Checkbox("Center##modifiertype", &center)) {
		command::executeCommands("togglestampbrushcenter", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcenter");
	bool continuous = brush.continuousMode();
	if (ImGui::Checkbox("Continuous##modifiertype", &continuous)) {
		command::executeCommands("togglestampbrushcontinuous", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcontinuous");

	if (_stampPaletteIndex >= 0 && _stampPaletteIndex < palette.colorCount()) {
		const float size = 20;
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(_stampPaletteIndex)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
	}
	ImGui::InputInt("##colorstampbrush", &_stampPaletteIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
			_stampPaletteIndex = *(const uint8_t *)payload->Data;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();
	if (ImGui::Button("replace")) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipText("Replace all voxels in the stamp with the selected color");

	if (ImGui::CollapsingHeader("Reduce size")) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputInt3("size##stampbrush", glm::value_ptr(size), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (glm::any(glm::greaterThan(size, region.getDimensionsInVoxels()))) {
				size = glm::min(size, region.getDimensionsInVoxels());
			}
			brush.setSize(size);
		}
	}
}

void BrushPanel::updatePlaneBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	if (modifier.modifierType() == ModifierType::Place) {
		ImGui::TextWrapped("Extrude voxels");
	} else if (modifier.modifierType() == ModifierType::Erase) {
		ImGui::TextWrapped("Erase voxels");
	} else if (modifier.modifierType() == ModifierType::Paint) {
		ImGui::TextWrapped("Paint voxels");
	} else if (modifier.modifierType() == (ModifierType::Paint | ModifierType::Erase)) {
		ImGui::TextWrapped("Override voxels");
	}
}

void BrushPanel::updateStampBrushPanel(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	palette::Palette &palette = node.palette();

	Modifier &modifier = sceneMgr().modifier();
	if (!modifier.stampBrush().active()) {
		ImGui::TextWrapped("Select a model from the asset panel");
		ui::ScopedStyle style;
		style.disableItem();
		stampBrushOptions(node, palette, listener);
	} else {
		stampBrushOptions(node, palette, listener);
	}
	stampBrushUseSelection(node, palette);
	if (ImGui::Button("Convert palette")) {
		modifier.stampBrush().convertToPalette(palette);
	}
}

void BrushPanel::updateShapeBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	addShapes(listener);
	addMirrorPlanes(listener);
	bool center = modifier.shapeBrush().centerMode();
	if (ImGui::Checkbox("Center##modifiertype", &center)) {
		command::executeCommands("toggleshapebrushcenter", &listener);
	}
	ImGui::TooltipCommand("toggleshapebrushcenter");
	bool single = modifier.shapeBrush().singleMode();
	if (ImGui::Checkbox("Single##modifiertype", &single)) {
		command::executeCommands("toggleshapebrushsingle", &listener);
	}
	ImGui::TooltipCommand("toggleshapebrushsingle");
}

void BrushPanel::brushRegion() {
	Modifier &modifier = sceneMgr().modifier();
	ShapeBrush *brush = modifier.activeShapeBrush();
	if (brush == nullptr) {
		return;
	}
	if (ImGui::CollapsingHeader("Brush region")) {
		ui::ScopedStyle style;
		if (brush->singleMode()) {
			style.disableItem();
		}
		voxel::Region region = modifier.calcBrushRegion();
		glm::ivec3 mins = region.getLowerCorner();
		glm::ivec3 maxs = region.getUpperCorner();
		if (ImGui::InputInt3("mins##regionbrush", glm::value_ptr(mins))) {
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		if (ImGui::InputInt3("maxs##regionbrush", glm::value_ptr(maxs))) {
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		if (ImGui::Button(ICON_LC_ARROW_RIGHT)) {
			_face = voxel::FaceNames::PositiveX;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in positive x direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_ARROW_LEFT)) {
			_face = voxel::FaceNames::NegativeX;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in negative x direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_ARROW_UP)) {
			_face = voxel::FaceNames::PositiveY;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in positive y direction");
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_ARROW_DOWN)) {
			_face = voxel::FaceNames::NegativeY;
			brush->setRegion(voxel::Region(mins, maxs), _face);
		}
		ImGui::TooltipText("Generate in negative y direction");
	}
}

void BrushPanel::brushSettings(command::CommandExecutionListener &listener) {
	const Modifier &modifier = sceneMgr().modifier();
	if (ImGui::CollapsingHeader("Brush settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (modifier.brushType() == BrushType::Shape) {
			updateShapeBrushPanel(listener);
		} else if (modifier.brushType() == BrushType::Stamp) {
			updateStampBrushPanel(listener);
		} else if (modifier.brushType() == BrushType::Plane) {
			updatePlaneBrushPanel(listener);
		}
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.setFont(imguiApp()->bigIconFont());
	const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
	ui::Toolbar toolbar(buttonSize, &listener);
	const voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	const bool moverride = modifier.isMode(ModifierType::Place | ModifierType::Erase);
	const bool mplace = !moverride && modifier.isMode(ModifierType::Place);
	const bool merase = !moverride && modifier.isMode(ModifierType::Erase);

	toolbar.button(ICON_LC_BOX, "actionplace", !mplace);
	toolbar.button(ICON_LC_ERASER, "actionerase", !merase);
	toolbar.button(ICON_LC_PEN_SQUARE, "actionoverride", !moverride);
	toolbar.button(ICON_LC_PAINTBRUSH, "actionpaint", !modifier.isMode(ModifierType::Paint));
	toolbar.button(ICON_LC_EXPAND, "actionselect", !modifier.isMode(ModifierType::Select));
	toolbar.button(ICON_LC_PEN_LINE, "actionpath", !modifier.isMode(ModifierType::Path));
	toolbar.button(ICON_LC_PEN_LINE, "actionline", !modifier.isMode(ModifierType::Line));
	toolbar.button(ICON_LC_PIPETTE, "actioncolorpicker", !modifier.isMode(ModifierType::ColorPicker));
}

void BrushPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		addModifiers(listener);
	}

	Modifier &modifier = sceneMgr().modifier();
	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrapped("Click on a voxel to pick the color");
	} else if (modifier.isMode(ModifierType::Line)) {
		ImGui::TextWrapped("Draws a line from the reference position to the current cursor position");
	} else if (modifier.isMode(ModifierType::Path)) {
		ImGui::TextWrapped("Draws a path over existing voxels");
	} else if (modifier.isMode(ModifierType::Select)) {
		ImGui::TextWrapped("Select areas of voxels");
	} else {
		const int currentBrush = (int)modifier.brushType();
		const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
		ui::Toolbar toolbar(buttonSize, &listener);
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			auto func = [&modifier, i]() { modifier.setBrushType((BrushType)i); };
			toolbar.button(BrushTypeIcons[i], BrushTypeStr[i], func, currentBrush != i);
		}
		toolbar.end();

		brushRegion();
		brushSettings(listener);

		ImGui::Separator();
	}
	ImGui::End();
}

} // namespace voxedit
