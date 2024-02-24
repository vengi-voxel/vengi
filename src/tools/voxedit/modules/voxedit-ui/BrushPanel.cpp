/**
 * @file
 */

#include "BrushPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "Toolbar.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/ScopedPtr.h"
#include "imgui.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/Util.h"
#include "voxedit-util/Clipboard.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static constexpr const char *BrushTypeIcons[] = {ICON_LC_MOUSE_POINTER_SQUARE_DASHED,
												 ICON_LC_BOXES,
												 ICON_LC_GROUP,
												 ICON_LC_STAMP,
												 ICON_LC_PEN_LINE,
												 ICON_LC_FOOTPRINTS,
												 ICON_LC_PAINTBRUSH};
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

bool BrushPanel::mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener,
									   AABBBrush &brush) {
	core::String cmd = "mirroraxis" + brush.name().toLower() +
					   "brush"; // mirroraxisshapebrushx, mirroraxisshapebrushy, mirroraxisshapebrushz
	cmd += math::getCharForAxis(type);
	{
		ui::ScopedStyle style;
		veui::AxisStyleText(style, type, false);
		if (ImGui::RadioButton(title, brush.mirrorAxis() == type)) {
			command::executeCommands(cmd, &listener);
			return true;
		}
	}
	const core::String &help = command::help(cmd);
	if (!help.empty()) {
		ImGui::TooltipText("%s", help.c_str());
	}
	return false;
}

void BrushPanel::addMirrorPlanes(command::CommandExecutionListener &listener, AABBBrush &brush) {
	mirrorAxisRadioButton(_("Disable mirror##mirror"), math::Axis::None, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("X##mirror"), math::Axis::X, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Y##mirror"), math::Axis::Y, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Z##mirror"), math::Axis::Z, listener, brush);
}

void BrushPanel::stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette) {
	Modifier &modifier = sceneMgr().modifier();
	ui::ScopedStyle selectionStyle;
	if (modifier.selections().empty()) {
		selectionStyle.disableItem();
	}
	if (ImGui::Button(_("Use selection"))) {
		const Selections &selections = modifier.selections();
		if (!selections.empty()) {
			core::ScopedPtr<voxel::RawVolume> copy(voxedit::tool::copy(node.volume(), selections));
			if (copy) {
				modifier.stampBrush().setVolume(*copy, palette);
			}
		}
	}
	ImGui::TooltipText(_("Use the current selection as new stamp"));
}

void BrushPanel::stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	StampBrush &brush = modifier.stampBrush();
	ImGui::InputTextWithHint(_("Model"), _("Select a model from the asset panel"), &_stamp, ImGuiInputTextFlags_ReadOnly);
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
	if (ImGui::Checkbox(_("Center##modifiertype"), &center)) {
		command::executeCommands("togglestampbrushcenter", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcenter");
	bool continuous = brush.continuousMode();
	if (ImGui::Checkbox(_("Continuous##modifiertype"), &continuous)) {
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
	if (ImGui::Button(_("Replace##stampbrush"))) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipText(_("Replace all voxels in the stamp with the selected color"));

	if (ImGui::CollapsingHeader(_("Reduce size"))) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputInt3(_("Size##stampbrush"), glm::value_ptr(size), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (glm::any(glm::greaterThan(size, region.getDimensionsInVoxels()))) {
				size = glm::min(size, region.getDimensionsInVoxels());
			}
			brush.setSize(size);
		}
	}
}

void BrushPanel::updatePlaneBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	if (modifier.isMode(ModifierType::Place)) {
		ImGui::TextWrapped(_("Extrude voxels"));
	} else if (modifier.isMode(ModifierType::Erase)) {
		ImGui::TextWrapped(_("Erase voxels"));
	} else if (modifier.isMode(ModifierType::Override)) {
		ImGui::TextWrapped(_("Override voxels"));
	}
}

void BrushPanel::updateLineBrushPanel(command::CommandExecutionListener &listener) {
	ImGui::TextWrapped(_("Draws a line from the reference position to the current cursor position"));
}

void BrushPanel::updatePathBrushPanel(command::CommandExecutionListener &listener) {
	ImGui::TextWrapped(_("Draws a path over existing voxels"));
}

void BrushPanel::updateStampBrushPanel(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	palette::Palette &palette = node.palette();

	Modifier &modifier = sceneMgr().modifier();
	if (!modifier.stampBrush().active()) {
		ImGui::TextWrapped(_("Select a model from the asset panel"));
		ui::ScopedStyle style;
		style.disableItem();
		stampBrushOptions(node, palette, listener);
	} else {
		stampBrushOptions(node, palette, listener);
	}
	stampBrushUseSelection(node, palette);
	if (ImGui::Button(_("Convert palette"))) {
		modifier.stampBrush().convertToPalette(palette);
	}
}

void BrushPanel::aabbBrushOptions(command::CommandExecutionListener &listener, AABBBrush &brush) {
	addMirrorPlanes(listener, brush);

	ImGui::Separator();

	const bool aabb = brush.aabbMode();
	core::String toggleAABBCmd = "set" + brush.name().toLower() + "brushaabb";
	ImGui::CommandRadioButton(_("Default"), toggleAABBCmd, aabb, &listener);

	const bool single = brush.singleMode();
	core::String toggleSingleCmd = "set" + brush.name().toLower() + "brushsingle";
	ImGui::CommandRadioButton(_("Single"), toggleSingleCmd, single, &listener);

	const bool center = brush.centerMode();
	core::String toggleCenterCmd = "set" + brush.name().toLower() + "brushcenter";
	ImGui::CommandRadioButton(_("Center"), toggleCenterCmd, center, &listener);
}

// doing this after aabbBrushOptions() allows us to extend the radio buttons
void BrushPanel::aabbBrushModeOptions(AABBBrush &brush) {
	if (brush.singleMode()) {
		int radius = brush.radius();
		if (ImGui::InputInt(_("Radius"), &radius)) {
			brush.setRadius(radius);
		}
		ImGui::TooltipText(_("Use a radius around the current voxel - 0 for spanning a region"));
	}
}

void BrushPanel::updateShapeBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	ShapeBrush &brush = modifier.shapeBrush();
	addShapes(listener);
	aabbBrushOptions(listener, brush);
	aabbBrushModeOptions(brush);
}

void BrushPanel::updatePaintBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = sceneMgr().modifier();
	PaintBrush &brush = modifier.paintBrush();

	PaintBrush::PaintMode paintMode = brush.paintMode();
	if (ImGui::BeginCombo(_("Mode"), PaintBrush::PaintModeStr[(int)paintMode], ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)PaintBrush::PaintMode::Max; ++i) {
			const PaintBrush::PaintMode mode = (PaintBrush::PaintMode)i;
			const bool selected = mode == paintMode;
			if (ImGui::Selectable(PaintBrush::PaintModeStr[i], selected)) {
				brush.setPaintMode(mode);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (paintMode == PaintBrush::PaintMode::Brighten || paintMode == PaintBrush::PaintMode::Darken ||
		paintMode == PaintBrush::PaintMode::Variation) {
		float factor = brush.factor();
		if (ImGui::InputFloat(_("Factor"), &factor)) {
			brush.setFactor(factor);
		}
	}
	if (paintMode == PaintBrush::PaintMode::Variation) {
		int variationThreshold = brush.variationThreshold();
		if (ImGui::InputInt(_("Variation threshold"), &variationThreshold)) {
			brush.setVariationThreshold(variationThreshold);
		}
	}

	aabbBrushOptions(listener, brush);
	if (ImGui::RadioButton(_("Plane"), brush.plane())) {
		brush.setPlane();
	}
	ImGui::TooltipText(_("Paint the selected plane"));
	aabbBrushModeOptions(brush);
}

void BrushPanel::brushSettings(command::CommandExecutionListener &listener) {
	const Modifier &modifier = sceneMgr().modifier();
	const BrushType brushType = modifier.brushType();
	if (ImGui::CollapsingHeader(_("Brush settings"), ImGuiTreeNodeFlags_DefaultOpen)) {
		if (brushType == BrushType::Shape) {
			updateShapeBrushPanel(listener);
		} else if (brushType == BrushType::Stamp) {
			updateStampBrushPanel(listener);
		} else if (brushType == BrushType::Plane) {
			updatePlaneBrushPanel(listener);
		} else if (brushType == BrushType::Line) {
			updateLineBrushPanel(listener);
		} else if (brushType == BrushType::Path) {
			updatePathBrushPanel(listener);
		} else if (brushType == BrushType::Paint) {
			updatePaintBrushPanel(listener);
		}
	}
	if (brushType == BrushType::None) {
		if (modifier.isMode(ModifierType::ColorPicker)) {
			ImGui::TextWrapped(_("Click on a voxel to pick the color"));
		} else if (modifier.isMode(ModifierType::Select)) {
			ImGui::TextWrapped(_("Select areas of voxels"));
		}
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.setFont(_app->bigIconFont());

	const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	const BrushType brushType = modifier.brushType();

	ui::Toolbar toolbarBrush(buttonSize, &listener);
	for (int i = 0; i < (int)BrushType::Max; ++i) {
		core::String cmd = core::string::format("brush%s", BrushTypeStr[i]).toLower();
		auto func = [&listener, cmd]() { command::executeCommands(cmd, &listener); };
		core::String tooltip = command::help(cmd);
		if (tooltip.empty()) {
			tooltip = BrushTypeStr[i];
		}
		toolbarBrush.button(BrushTypeIcons[i], tooltip.c_str(), func, (int)brushType != i);
	}
	toolbarBrush.end();

	if (brushType == BrushType::Paint) {
		return;
	}

	ui::Toolbar toolbarModifiers(buttonSize, &listener);
	if (brushType == BrushType::None) {
		toolbarModifiers.button(ICON_LC_EXPAND, "actionselect", !modifier.isMode(ModifierType::Select));
		toolbarModifiers.button(ICON_LC_PIPETTE, "actioncolorpicker", !modifier.isMode(ModifierType::ColorPicker));
	} else {
		toolbarModifiers.button(ICON_LC_BOX, "actionplace", !modifier.isMode(ModifierType::Place));
		toolbarModifiers.button(ICON_LC_ERASER, "actionerase", !modifier.isMode(ModifierType::Erase));
		toolbarModifiers.button(ICON_LC_SQUARE_PEN, "actionoverride", !modifier.isMode(ModifierType::Override));
	}
}

void BrushPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		addModifiers(listener);
		brushSettings(listener);
	}
	ImGui::End();
}

} // namespace voxedit
