/**
 * @file
 */

#include "BrushPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "Toolbar.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Bits.h"
#include "core/Enum.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/Util.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

static constexpr const char *BrushTypeIcons[] = {ICON_LC_SQUARE_DASHED_MOUSE_POINTER,
												 ICON_LC_BOXES,
												 ICON_LC_GROUP,
												 ICON_LC_STAMP,
												 ICON_LC_PEN_LINE,
												 ICON_LC_FOOTPRINTS,
												 ICON_LC_PAINTBRUSH,
												 ICON_LC_TEXT};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

void BrushPanel::addShapes(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();

	const ShapeType currentSelectedShapeType = modifier.shapeBrush().shapeType();
	if (ImGui::BeginCombo(_("Shape"), ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
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
									   Brush &brush) {
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

void BrushPanel::addMirrorPlanes(command::CommandExecutionListener &listener, Brush &brush) {
	ImGui::PushID("##mirrorplanes");
	mirrorAxisRadioButton(_("Disable mirror"), math::Axis::None, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("X"), math::Axis::X, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Y"), math::Axis::Y, listener, brush);
	ImGui::SameLine();
	mirrorAxisRadioButton(_("Z"), math::Axis::Z, listener, brush);
	ImGui::PopID();
}

void BrushPanel::stampBrushUseSelection(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	ui::ScopedStyle selectionStyle;
	if (modifier.selections().empty()) {
		selectionStyle.disableItem();
	}
	ImGui::CommandButton(_("Use selection"), "stampbrushuseselection", listener);
}

void BrushPanel::stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	StampBrush &brush = modifier.stampBrush();
	addMirrorPlanes(listener, brush);
	ImGui::Separator();
	ImGui::InputTextWithHint(_("Model"), _("Select a model from the asset panel or scene graph panel"), &_stamp, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			if (brush.load(filename)) {
				_stamp = filename;
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::SceneNodePayload)) {
			int nodeId = *(int *)payload->Data;
			if (const scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(nodeId)) {
				brush.setVolume(*model->volume(), model->palette());
			}
		}
		ImGui::EndDragDropTarget();
	}

	bool center = brush.centerMode();
	if (ImGui::Checkbox(_("Center"), &center)) {
		command::executeCommands("togglestampbrushcenter", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcenter");
	bool continuous = brush.continuousMode();
	if (ImGui::Checkbox(_("Continuous"), &continuous)) {
		command::executeCommands("togglestampbrushcontinuous", &listener);
	}
	ImGui::TooltipCommand("togglestampbrushcontinuous");
	addBrushClampingOption(brush);

	if (_stampPaletteIndex >= 0 && _stampPaletteIndex < palette.colorCount()) {
		const float size = 20;
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(palette.uiIndex(_stampPaletteIndex))));
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
	if (ImGui::Button(_("Replace"))) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipTextUnformatted(_("Replace all voxels in the stamp with the selected color"));

	const float buttonWidth = (float)_app->fontSize() * 4;
	if (ImGui::CollapsingHeader(_("Rotate on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##rotateonaxis");
		veui::AxisButton(math::Axis::X, _("X"), "stampbrushrotate x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the x axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Y, _("Y"), "stampbrushrotate y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the y axis"));
		ImGui::SameLine();
		veui::AxisButton(math::Axis::Z, _("Z"), "stampbrushrotate z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
		ImGui::TooltipTextUnformatted(_("Rotate by 90 degree on the z axis"));
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(_("Reduce size"))) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputInt3(_("Size"), glm::value_ptr(size), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (glm::any(glm::greaterThan(size, region.getDimensionsInVoxels()))) {
				size = glm::min(size, region.getDimensionsInVoxels());
			}
			brush.setSize(size);
		}
	}
}

void BrushPanel::updatePlaneBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	if (modifier.isMode(ModifierType::Place)) {
		ImGui::TextWrappedUnformatted(_("Extrude voxels"));
	} else if (modifier.isMode(ModifierType::Erase)) {
		ImGui::TextWrappedUnformatted(_("Erase voxels"));
	} else if (modifier.isMode(ModifierType::Override)) {
		ImGui::TextWrappedUnformatted(_("Override voxels"));
	}
}

void BrushPanel::updateLineBrushPanel(command::CommandExecutionListener &listener) {
	ImGui::TextWrappedUnformatted(_("Draws a line from the reference position to the current cursor position"));
}

void BrushPanel::updatePathBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	PathBrush &brush = modifier.pathBrush();
	voxelutil::Connectivity c = brush.connectivity();
	int selected = (int)c;
	const char *items[] = {_("6-connected"), _("18-connected"), _("26-connected")};
	if (ImGui::BeginCombo(_("Connectivity"), items[selected])) {
		for (int i = 0; i < lengthof(items); ++i) {
			bool isSelected = selected == i;
			if (ImGui::Selectable(items[i], isSelected)) {
				brush.setConnectivity((voxelutil::Connectivity)i);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::TextWrappedUnformatted(_("Draws a path over existing voxels"));
}

void BrushPanel::updateStampBrushPanel(command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	palette::Palette &palette = node.palette();

	Modifier &modifier = _sceneMgr->modifier();
	if (!modifier.stampBrush().active()) {
		ImGui::TextWrappedUnformatted(_("Select a model from the asset panel or scene graph panel"));
		ui::ScopedStyle style;
		style.disableItem();
		stampBrushOptions(node, palette, listener);
	} else {
		stampBrushOptions(node, palette, listener);
	}

	stampBrushUseSelection(node, palette, listener);
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
		ImGui::TooltipTextUnformatted(_("Use a radius around the current voxel - 0 for spanning a region"));
	}
}

void BrushPanel::addBrushClampingOption(Brush &brush) {
	bool clamping = brush.brushClamping();
	if (ImGui::Checkbox(_("Clamping"), &clamping)) {
		brush.setBrushClamping(clamping);
	}
	ImGui::TooltipTextUnformatted(_("Clamp the brush to the volume"));
}

void BrushPanel::updateShapeBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	ShapeBrush &brush = modifier.shapeBrush();
	addShapes(listener);
	aabbBrushOptions(listener, brush);
	aabbBrushModeOptions(brush);
}

void BrushPanel::updateTextBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	TextBrush &brush = modifier.textBrush();
	if (ImGui::InputText(_("Text"), &brush.input())) {
		brush.markDirty();
	}

	ImGui::SetNextItemWidth(100.0f);
	int size = brush.size();
	if (ImGui::InputInt(ICON_LC_MOVE_VERTICAL, &size)) {
		brush.setSize(size);
	}
	ImGui::TooltipTextUnformatted(_("Font size"));
	ImGui::SameLine();

	ImGui::SetNextItemWidth(100.0f);
	int spacing = brush.spacing();
	if (ImGui::InputInt(ICON_LC_MOVE_HORIZONTAL "##textinput", &spacing)) {
		brush.setSpacing(spacing);
	}
	ImGui::TooltipTextUnformatted(_("Horizontal spacing"));

	int thickness = brush.thickness();
	if (ImGui::InputInt(ICON_LC_EXPAND "##textinput", &thickness)) {
		brush.setThickness(thickness);
	}
	ImGui::TooltipTextUnformatted(_("Thickness"));

	const float buttonWidth = (float)_app->fontSize() * 4;
	veui::AxisButton(math::Axis::X, _("X"), "textbrushaxis x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::TooltipTextUnformatted(_("Print text along the x axis"));
	ImGui::SameLine();
	veui::AxisButton(math::Axis::Y, _("Y"), "textbrushaxis y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::TooltipTextUnformatted(_("Print text along the y axis"));
	ImGui::SameLine();
	veui::AxisButton(math::Axis::Z, _("Z"), "textbrushaxis z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::TooltipTextUnformatted(_("Print text along the z axis"));

	addMirrorPlanes(listener, modifier.textBrush());
	ImGui::Separator();
	addBrushClampingOption(brush);

	if (ImGui::InputFile(_("Font"), &brush.font(), io::format::fonts(), ImGuiInputTextFlags_ReadOnly)) {
		brush.markDirty();
	}
}

void BrushPanel::updatePaintBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
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
	ImGui::TooltipTextUnformatted(_("Paint the selected plane"));

	if (ImGui::RadioButton(_("Gradient"), brush.gradient())) {
		brush.setGradient();
	}

	aabbBrushModeOptions(brush);
}

void BrushPanel::brushSettings(command::CommandExecutionListener &listener) {
	const Modifier &modifier = _sceneMgr->modifier();
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
		} else if (brushType == BrushType::Text) {
			updateTextBrushPanel(listener);
		}
	}
	if (brushType == BrushType::None) {
		if (modifier.isMode(ModifierType::ColorPicker)) {
			ImGui::TextWrappedUnformatted(_("Click on a voxel to pick the color"));
		} else if (modifier.isMode(ModifierType::Select)) {
			ImGui::TextWrappedUnformatted(_("Select areas of voxels"));
		}
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.setFont(_app->bigIconFont());

	const ImVec2 buttonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());

	voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();

	ui::Toolbar toolbarBrush("brushes", buttonSize, &listener);
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

	const ModifierType supported = modifier.checkModifierType();
	if (core::countSetBits(core::enumVal(supported)) > 1) {
		ui::Toolbar toolbarModifiers("modifiers", buttonSize, &listener);
		if ((supported & ModifierType::Select) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_EXPAND, "actionselect", !modifier.isMode(ModifierType::Select));
		}
		if ((supported & ModifierType::ColorPicker) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_PIPETTE, "actioncolorpicker", !modifier.isMode(ModifierType::ColorPicker));
		}
		if ((supported & ModifierType::Place) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_BOX, "actionplace", !modifier.isMode(ModifierType::Place));
		}
		if ((supported & ModifierType::Erase) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_ERASER, "actionerase", !modifier.isMode(ModifierType::Erase));
		}
		if ((supported & ModifierType::Override) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_SQUARE_PEN, "actionoverride", !modifier.isMode(ModifierType::Override));
		}
	}
}

void BrushPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanel);
	const core::String title = makeTitle(ICON_LC_BRUSH, _("Brush"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		addModifiers(listener);
		brushSettings(listener);
	}
	ImGui::End();
}

} // namespace voxedit
