/**
 * @file
 */

#include "BrushPanel.h"
#include "DragAndDropPayload.h"
#include "ScopedStyle.h"
#include "Style.h"
#include "Toolbar.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Bits.h"
#include "core/Enum.h"
#include "core/StringUtil.h"
#include "dearimgui/imgui_internal.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/ViewMode.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/LineBrush.h"
#include "voxedit-util/modifier/brush/NormalBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxedit-util/modifier/brush/ExtrudeBrush.h"
#include "voxedit-util/modifier/brush/TransformBrush.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxedit-util/modifier/brush/TextureBrush.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

namespace voxedit {

static constexpr const char *BrushTypeIcons[] = {
	ICON_LC_PIPETTE,	ICON_LC_BOXES,	   ICON_LC_GROUP,
	ICON_LC_STAMP,		ICON_LC_PEN_LINE,  ICON_LC_FOOTPRINTS,
	ICON_LC_PAINTBRUSH, ICON_LC_TEXT_WRAP, ICON_LC_SQUARE_DASHED_MOUSE_POINTER,
	ICON_LC_IMAGE,		ICON_LC_MOVE_UP_RIGHT, ICON_LC_EXPAND, ICON_LC_MOVE_3D, ICON_LC_BLEND};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

static constexpr const char *TransformModeStr[] = {NC_("Transform Modes", "Move"), NC_("Transform Modes", "Shear"),
												   NC_("Transform Modes", "Scale"), NC_("Transform Modes", "Rotate")};
static_assert(lengthof(TransformModeStr) == (int)TransformMode::Max, "TransformModeStr size mismatch");

static constexpr const char *SculptModeStr[] = {NC_("Sculpt Modes", "Erode"), NC_("Sculpt Modes", "Grow"), NC_("Sculpt Modes", "Flatten"), NC_("Sculpt Modes", "Smooth Additive"), NC_("Sculpt Modes", "Smooth Erode")};
static_assert(lengthof(SculptModeStr) == (int)SculptMode::Max, "SculptModeStr size mismatch");

static constexpr const char *VoxelSamplingStr[] = {NC_("Scale Sampling", "Nearest"), NC_("Scale Sampling", "Linear"),
												   NC_("Scale Sampling", "Cubic")};
static_assert(lengthof(VoxelSamplingStr) == (int)voxel::VoxelSampling::Max, "VoxelSamplingStr size mismatch");

static constexpr size_t DeferredTransformThreshold = 10000;

void BrushPanel::init() {
	_renderNormals = core::getVar(cfg::RenderNormals);
	_viewMode = core::getVar(cfg::VoxEditViewMode);
}

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

	if (currentSelectedShapeType == ShapeType::Circle) {
		int thickness = modifier.shapeBrush().thickness();
		if (ImGui::InputInt(_("Thickness"), &thickness)) {
			modifier.shapeBrush().setThickness(thickness);
		}
	}
}

bool BrushPanel::mirrorAxisRadioButton(const char *title, math::Axis type, command::CommandExecutionListener &listener,
									   Brush &brush) {
	core::String cmd = "mirroraxis" + brush.name().toLower() +
					   "brush"; // mirroraxisshapebrushx, mirroraxisshapebrushy, mirroraxisshapebrushz
	cmd += math::getCharForAxis(type);
	{
		ui::ScopedStyle style;
		ImGui::AxisStyleText(style, type);
		if (ImGui::RadioButton(title, brush.mirrorAxis() == type)) {
			command::executeCommands(cmd, &listener);
			return true;
		}
	}
	const core::String &help = command::help(cmd);
	if (!help.empty()) {
		ImGui::TooltipTextUnformatted(help.c_str());
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
	ui::ScopedStyle selectionStyle;
	ImGui::BeginDisabled(!node.hasSelection());
	ImGui::CommandButton(_("Use selection"), "stampbrushuseselection", listener);
	ImGui::EndDisabled();
}

void BrushPanel::stampBrushOptions(scenegraph::SceneGraphNode &node, palette::Palette &palette,
								   command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	StampBrush &brush = modifier.stampBrush();
	addMirrorPlanes(listener, brush);
	ImGui::Separator();
	ImGui::InputTextWithHint(_("Model"), _("Select a model from the asset panel or scene graph panel"), &_stamp,
							 ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ModelPayload)) {
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

	glm::ivec3 offset = brush.offset();
	if (ImGui::InputXYZ(_("Offset"), offset)) {
		brush.setOffset(offset);
	}

	addBrushClampingOption(brush);

	if (_stampPaletteIndex >= 0 && _stampPaletteIndex < palette.colorCount()) {
		const float size = ImGui::Height(1);
		const ImVec2 v1 = ImGui::GetCursorScreenPos();
		const ImVec2 v2(v1.x + size, v1.y + size);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const uint32_t col = ImGui::GetColorU32(palette.color(palette.view().uiIndex(_stampPaletteIndex)));
		drawList->AddRectFilled(v1, v2, col);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
	}
	ImGui::InputInt("##colorstampbrush", &_stampPaletteIndex, 0, 0, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
			_stampPaletteIndex = *(const uint8_t *)payload->Data;
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine();
	if (ImGui::Button(_("Replace"))) {
		brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, _stampPaletteIndex), palette);
	}
	ImGui::TooltipTextUnformatted(_("Replace all voxels in the stamp with the selected color"));

	const float buttonWidth = ImGui::GetFontSize() * 4.0f;
	if (ImGui::CollapsingHeader(_("Rotate on axis"), ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID("##rotateonaxis");
		ImGui::AxisCommandButton(math::Axis::X, _("X"), "stampbrushrotate x", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "stampbrushrotate y", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::SameLine();
		ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "stampbrushrotate z", ICON_LC_REPEAT, nullptr, buttonWidth,
								 &listener);
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(_("Reduce size"))) {
		voxel::Region region = brush.volume()->region();
		glm::ivec3 size = region.getDimensionsInVoxels();
		if (ImGui::InputXYZ(_("Size"), size, nullptr, ImGuiInputTextFlags_EnterReturnsTrue)) {
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
	Modifier &modifier = _sceneMgr->modifier();
	LineBrush &brush = modifier.lineBrush();
	bool continuous = brush.continuous();
	if (ImGui::Checkbox(_("Continuous"), &continuous)) {
		brush.setContinuous(continuous);
	}
	ImGui::TooltipCommand("togglelinebrushcontinuous");

	ImGui::TextUnformatted(_("Stipple Pattern:"));
	LineStipplePattern &stipplePattern = brush.stipplePattern();
	ui::ScopedStyle style;
	style.setItemSpacing({0.0f, 0.0f});
	for (int i = 0; i < stipplePattern.bits(); ++i) {
		ImGui::PushID(i);
		bool bit = stipplePattern[i];
		if (ImGui::Checkbox("", &bit)) {
			brush.setStippleBit(i, bit);
		}
		ImGui::PopID();
		ImGui::SameLine();
	}
	ImGui::TooltipTextUnformatted(_("Length of the stipple pattern <= 1 to disable"));
}

void BrushPanel::updateSelectBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	SelectBrush &brush = modifier.selectBrush();

	int selectModeInt = (int)brush.selectMode();

	const char *SelectModeStr[] = {C_("SelectMode", "All"), C_("SelectMode", "Surface"), C_("SelectMode", "Same Color"),
								   C_("SelectMode", "Fuzzy Color"), C_("SelectMode", "Connected"),
								   C_("SelectMode", "Flat Surface"), C_("SelectMode", "3D Box"),
								   C_("SelectMode", "Circle"), C_("SelectMode", "Slope")};
	static_assert(lengthof(SelectModeStr) == (int)SelectMode::Max, "Array size mismatch");

	if (ImGui::Combo(_("Select mode"), &selectModeInt, SelectModeStr, (int)SelectMode::Max)) {
		brush.setSelectMode((SelectMode)selectModeInt);
	}

	if (brush.selectMode() == SelectMode::FuzzyColor) {
		float threshold = brush.colorThreshold();
		if (ImGui::SliderFloat(_("Threshold"), &threshold, color::ApproximationDistanceMin, color::ApproximationDistanceLoose, "%.0f")) {
			brush.setColorThreshold(threshold);
		}
		ImGui::TooltipTextUnformatted(_("Color distance threshold for fuzzy matching (0 = exact, higher = more similar colors)"));
	}

	if (brush.selectMode() == SelectMode::FlatSurface) {
		int deviation = brush.flatDeviation();
		const float btnW = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::TextUnformatted(_("Accepted deviation"));
		ImGui::TooltipTextUnformatted(_("How many voxels above or below the clicked face the fill may deviate from the start position"));
		ImGui::PushID("flatdeviation");
		if (ImGui::Button("-", ImVec2(btnW, 0))) {
			deviation = glm::max(deviation - 1, 0);
			brush.setFlatDeviation(deviation);
		}
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
		if (ImGui::SliderInt("##flatdeviation", &deviation, 0, SelectBrush::MaxFlatDeviation)) {
			brush.setFlatDeviation(deviation);
		}
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("+", ImVec2(btnW, 0))) {
			deviation = glm::min(deviation + 1, SelectBrush::MaxFlatDeviation);
			brush.setFlatDeviation(deviation);
		}
		ImGui::PopID();
	}

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);

	if (brush.selectMode() == SelectMode::Slope) {
		const float btnW = ImGui::GetFrameHeight();
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		bool changed = false;

		int deviation = brush.slopeDeviation();
		ImGui::TextUnformatted(_("Max deviation"));
		ImGui::TooltipTextUnformatted(_("Maximum height deviation (in voxels) from the fitted slope plane for a voxel to be included in the selection"));
		ImGui::PushID("slopedeviation");
		if (ImGui::Button("-", ImVec2(btnW, 0))) {
			deviation = glm::max(deviation - 1, 0);
			brush.setSlopeDeviation(deviation);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
		if (ImGui::SliderInt("##slopedeviation", &deviation, 0, SelectBrush::MaxSlopeDeviation)) {
			brush.setSlopeDeviation(deviation);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("+", ImVec2(btnW, 0))) {
			deviation = glm::min(deviation + 1, SelectBrush::MaxSlopeDeviation);
			brush.setSlopeDeviation(deviation);
			changed = true;
		}
		ImGui::PopID();

		int sampleDist = brush.slopeSampleDistance();
		ImGui::TextUnformatted(_("Sample distance"));
		ImGui::TooltipTextUnformatted(_("How far apart (in voxels) to sample when computing the initial slope plane. Larger values give smoother slope detection on staircases"));
		ImGui::PushID("slopesample");
		if (ImGui::Button("-", ImVec2(btnW, 0))) {
			sampleDist = glm::max(sampleDist - 1, SelectBrush::MinSlopeSampleDistance);
			brush.setSlopeSampleDistance(sampleDist);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
		if (ImGui::SliderInt("##slopesample", &sampleDist, SelectBrush::MinSlopeSampleDistance, SelectBrush::MaxSlopeSampleDistance)) {
			brush.setSlopeSampleDistance(sampleDist);
			changed = true;
		}
		ImGui::SameLine(0, spacing);
		if (ImGui::Button("+", ImVec2(btnW, 0))) {
			sampleDist = glm::min(sampleDist + 1, SelectBrush::MaxSlopeSampleDistance);
			brush.setSlopeSampleDistance(sampleDist);
			changed = true;
		}
		ImGui::PopID();

		if (changed && brush.slopeValid()) {
			_sceneMgr->selectionSetSlope(nodeId);
		}
	}

	if (node && node->hasSelection() && brush.selectMode() == SelectMode::Circle && brush.ellipseValid()) {
		ImGui::SeparatorText(_("Circle selection"));
		const voxel::Region &vol = node->region();
		const glm::ivec3 &vMins = vol.getLowerCorner();
		const glm::ivec3 &vMaxs = vol.getUpperCorner();
		glm::ivec3 center = brush.ellipseCenter();
		int radiusU = brush.ellipseRadiusU();
		int radiusV = brush.ellipseRadiusV();
		bool changed = false;

		int uAxis;
		int vAxis;
		SelectBrush::ellipseAxes(brush.ellipseFace(), uAxis, vAxis);
		const char *axisNames[] = {"X", "Y", "Z"};

		auto ellipseSlider = [&changed](const char *id, int *val, int lo, int hi) {
			ImGui::PushID(id);
			const float btnW = ImGui::GetFrameHeight();
			const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			if (ImGui::Button("-", ImVec2(btnW, 0))) {
				*val = glm::max(*val - 1, lo);
				changed = true;
			}
			ImGui::SameLine(0, spacing);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
			changed |= ImGui::SliderInt("", val, lo, hi);
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("+", ImVec2(btnW, 0))) {
				*val = glm::min(*val + 1, hi);
				changed = true;
			}
			ImGui::PopID();
		};

		ImGui::TextUnformatted(_("Center"));
		ellipseSlider("##cx", &center[uAxis], vMins[uAxis], vMaxs[uAxis]);
		ellipseSlider("##cy", &center[vAxis], vMins[vAxis], vMaxs[vAxis]);

		// Max radius is half the volume dimension on each axis
		const int maxRadiusU = (vMaxs[uAxis] - vMins[uAxis]) / 2;
		const int maxRadiusV = (vMaxs[vAxis] - vMins[vAxis]) / 2;
		ImGui::Text(_("Radius %s"), axisNames[uAxis]);
		ellipseSlider("##ru", &radiusU, 0, maxRadiusU);
		ImGui::Text(_("Radius %s"), axisNames[vAxis]);
		ellipseSlider("##rv", &radiusV, 0, maxRadiusV);

		const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(brush.ellipseFace()));
		const int maxDepth = (vMaxs[faceAxisIdx] - vMins[faceAxisIdx]) / 2;
		int depth = brush.ellipseDepth();
		ImGui::Text(_("Depth %s"), axisNames[faceAxisIdx]);
		ImGui::TooltipTextUnformatted(_("How far from the center the selection extends along the face-normal axis (0 = single layer)"));
		ellipseSlider("##depth", &depth, 1, maxDepth);

		bool is3D = brush.ellipse3D();
		if (ImGui::Checkbox(_("3D ellipsoid"), &is3D)) {
			brush.setEllipse3D(is3D);
			changed = true;
		}
		ImGui::TooltipTextUnformatted(_("Select voxels in a 3D ellipsoid shape behind the clicked surface instead of a 2D ellipse with depth"));

		if (changed) {
			brush.setEllipseCenter(center);
			brush.setEllipseRadiusU(radiusU);
			brush.setEllipseRadiusV(radiusV);
			brush.setEllipseDepth(depth);
			_sceneMgr->selectionSetEllipse(nodeId);
		}
	}

	if (node && node->hasSelection() && brush.selectMode() == SelectMode::Box3D) {
		voxel::Region sel = _sceneMgr->selectionCalculateRegion(nodeId);
		if (sel.isValid()) {
			ImGui::SeparatorText(_("Selection bounds"));
			const voxel::Region &vol = node->region();
			const glm::ivec3 &vMins = vol.getLowerCorner();
			const glm::ivec3 &vMaxs = vol.getUpperCorner();
			glm::ivec3 mins = sel.getLowerCorner();
			glm::ivec3 maxs = sel.getUpperCorner();
			bool changed = false;

			// Slider with -/+ buttons for fine single-step adjustment
			auto selSlider = [&changed](const char *id, int *val, int lo, int hi) {
				ImGui::PushID(id);
				const float btnW = ImGui::GetFrameHeight();
				const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
				if (ImGui::Button("-", ImVec2(btnW, 0))) {
					*val = glm::max(*val - 1, lo);
					changed = true;
				}
				ImGui::SameLine(0, spacing);
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - spacing);
				changed |= ImGui::SliderInt("", val, lo, hi);
				ImGui::SameLine(0, spacing);
				if (ImGui::Button("+", ImVec2(btnW, 0))) {
					*val = glm::min(*val + 1, hi);
					changed = true;
				}
				ImGui::PopID();
			};

			if (ImGui::BeginTable("##selbounds", 3, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("##axis", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn(_("Min"));
				ImGui::TableSetupColumn(_("Max"));
				ImGui::TableHeadersRow();

				ImGui::TableNextColumn(); ImGui::AxisButtonX(ImVec2(ImGui::GetFrameHeight(), 0));
				ImGui::TableNextColumn(); selSlider("##minx", &mins.x, vMins.x, vMaxs.x);
				ImGui::TableNextColumn(); selSlider("##maxx", &maxs.x, vMins.x, vMaxs.x);

				ImGui::TableNextColumn(); ImGui::AxisButtonY(ImVec2(ImGui::GetFrameHeight(), 0));
				ImGui::TableNextColumn(); selSlider("##miny", &mins.y, vMins.y, vMaxs.y);
				ImGui::TableNextColumn(); selSlider("##maxy", &maxs.y, vMins.y, vMaxs.y);

				ImGui::TableNextColumn(); ImGui::AxisButtonZ(ImVec2(ImGui::GetFrameHeight(), 0));
				ImGui::TableNextColumn(); selSlider("##minz", &mins.z, vMins.z, vMaxs.z);
				ImGui::TableNextColumn(); selSlider("##maxz", &maxs.z, vMins.z, vMaxs.z);

				ImGui::EndTable();
			}

			if (changed) {
				// ensure min <= max after independent dragging
				const glm::ivec3 newMins = glm::min(mins, maxs);
				const glm::ivec3 newMaxs = glm::max(mins, maxs);
				_sceneMgr->selectionSetBounds(nodeId, voxel::Region(newMins, newMaxs));
			}

			const glm::ivec3 size = sel.getDimensionsInVoxels();
			ImGui::Text(_("Size: %d x %d x %d"), size.x, size.y, size.z);
		}
	}
}

void BrushPanel::updateNormalBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	NormalBrush &brush = modifier.normalBrush();
	if (!_renderNormals->boolVal()) {
		ImGui::TextWrappedUnformatted(_("Enable normal rendering to see your changes"));
	}
	// TODO: BRUSH: see https://github.com/vengi-voxel/vengi/issues/253

	NormalBrush::PaintMode paintMode = brush.paintMode();
	int paintModeInt = (int)paintMode;
	if (ImGui::Combo(_("Mode"), &paintModeInt, NormalBrush::PaintModeStr, (int)NormalBrush::PaintMode::Max)) {
		brush.setPaintMode((NormalBrush::PaintMode)paintModeInt);
	}
}

void BrushPanel::updateTextureBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	TextureBrush &brush = modifier.textureBrush();
	core::String name = brush.image() ? core::string::extractFilenameWithExtension(brush.image()->name()) : _("None");
	ImGui::InputText(_("Texture"), &name, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ImagePayload)) {
			const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
			brush.setImage(image);
		}
		ImGui::EndDragDropTarget();
	}
	const bool itemClicked = ImGui::IsItemClicked();
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_FILE_INPUT) || itemClicked) {
		_app->openDialog(
			[&](const core::String &filename, const io::FormatDescription *desc) {
				const image::ImagePtr &image = _texturePool->loadImage(filename);
				brush.setImage(image);
			},
			{}, io::format::images());
	}

	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
	const bool hasSelection = node && node->hasSelection();
	ImGui::BeginDisabled(!node->hasSelection());
	ImGui::CommandIconButton(ICON_LC_SCAN, _("Use selection"), "texturebrushfromface", listener);
	ImGui::EndDisabled();

	bool projectOntoSurface = brush.projectOntoSurface();
	if (ImGui::Checkbox(_("Project onto surface"), &projectOntoSurface)) {
		brush.setProjectOntoSurface(projectOntoSurface);
	}

	glm::vec2 uv0 = brush.uv0();
	glm::vec2 uv1 = brush.uv1();
	if (brush.image()) {
		const video::TexturePtr &texture = _texturePool->load(brush.image()->name());
		const glm::vec2 &imgSize = brush.image()->size();
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const glm::vec2 aspect(available.x / imgSize.x, available.y / imgSize.y);
		const float scale = core_min(aspect.x, aspect.y);
		const ImVec2 size = ImVec2(imgSize.x * scale, imgSize.y * scale);
		ImGui::InvisibleButton("#texturebrushimage", size);
		ImGui::AddImage(texture->handle(), uv0, uv1);
		ImGui::OpenPopupOnItemClick(POPUP_TITLE_UV_EDITOR, ImGuiPopupFlags_MouseButtonLeft);
	}
	if (ImGui::InputFloat2(_("UV0"), glm::value_ptr(uv0))) {
		brush.setUV0(uv0);
	}
	ImGui::TooltipTextUnformatted(_("Texture coordinates"));
	if (ImGui::InputFloat2(_("UV1"), glm::value_ptr(uv1))) {
		brush.setUV1(uv1);
	}
	ImGui::TooltipTextUnformatted(_("Texture coordinates"));
}

void BrushPanel::updatePathBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	PathBrush &brush = modifier.pathBrush();
	voxel::Connectivity c = brush.connectivity();
	int selected = (int)c;
	const char *items[] = {_("6-connected"), _("18-connected"), _("26-connected")};
	if (ImGui::BeginCombo(_("Connectivity"), items[selected])) {
		for (int i = 0; i < lengthof(items); ++i) {
			bool isSelected = selected == i;
			if (ImGui::Selectable(items[i], isSelected)) {
				brush.setConnectivity((voxel::Connectivity)i);
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
		ImGui::BeginDisabled();
		stampBrushOptions(node, palette, listener);
		ImGui::EndDisabled();
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

	const bool singleMove = brush.singleModeMove();
	core::String toggleSingleMoveCmd = "set" + brush.name().toLower() + "brushsinglemove";
	ImGui::CommandRadioButton(_("Single Move"), toggleSingleMoveCmd, singleMove, &listener);

	const bool center = brush.centerMode();
	core::String toggleCenterCmd = "set" + brush.name().toLower() + "brushcenter";
	ImGui::CommandRadioButton(_("Center"), toggleCenterCmd, center, &listener);
}

// doing this after aabbBrushOptions() allows us to extend the radio buttons
void BrushPanel::aabbBrushModeOptions(AABBBrush &brush) {
	if (brush.anySingleMode()) {
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

	ImGui::SetNextItemWidth(ImGui::Size(10.0f));
	int size = brush.size();
	if (ImGui::InputInt(ICON_LC_MOVE_VERTICAL, &size)) {
		brush.setSize(size);
	}
	ImGui::TooltipTextUnformatted(_("Font size"));
	ImGui::SameLine();

	ImGui::SetNextItemWidth(ImGui::Size(10.0f));
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

	const float buttonWidth = ImGui::GetFontSize() * 4.0f;
	ImGui::AxisCommandButton(math::Axis::X, _("X"), "textbrushaxis x", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::SameLine();
	ImGui::AxisCommandButton(math::Axis::Y, _("Y"), "textbrushaxis y", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);
	ImGui::SameLine();
	ImGui::AxisCommandButton(math::Axis::Z, _("Z"), "textbrushaxis z", ICON_LC_REPEAT, nullptr, buttonWidth, &listener);

	addMirrorPlanes(listener, modifier.textBrush());
	ImGui::Separator();
	addBrushClampingOption(brush);

	ImGui::InputFile(_("Font"), true, &brush.font(), io::format::fonts(), ImGuiInputTextFlags_ReadOnly);
	if (brush.font() != _lastFont) {
		_lastFont = brush.font();
		brush.markDirty();
	}
}

void BrushPanel::updatePaintBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	PaintBrush &brush = modifier.paintBrush();

	PaintBrush::PaintMode paintMode = brush.paintMode();
	if (ImGui::BeginCombo(_("Mode"), _(PaintBrush::PaintModeStr[(int)paintMode]), ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)PaintBrush::PaintMode::Max; ++i) {
			const PaintBrush::PaintMode mode = (PaintBrush::PaintMode)i;
			const bool selected = mode == paintMode;
			if (ImGui::Selectable(_(PaintBrush::PaintModeStr[i]), selected)) {
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

enum class UVEdge { UpperLeft, LowerRight, UpperRight, LowerLeft, Max };

static bool addUVHandle(UVEdge edge, const glm::ivec2 &mins, const glm::ivec2 &maxs, const glm::ivec2 &uiImageSize,
						uint32_t colorInt, float &u, float &v) {
	glm::ivec2 handlePos;
	switch (edge) {
	case UVEdge::UpperLeft:
		handlePos = mins;
		break;
	case UVEdge::LowerRight:
		handlePos = maxs;
		break;
	case UVEdge::UpperRight:
		handlePos = glm::ivec2(maxs.x, mins.y);
		break;
	case UVEdge::LowerLeft:
		handlePos = glm::ivec2(mins.x, maxs.y);
		break;
	default:
		return false;
	}
	const float size = ImGui::Size(1);
	const glm::ivec2 pos1(handlePos.x - size, handlePos.y - size);
	const glm::ivec2 pos2(handlePos.x + size, handlePos.y + size);
	bool retVal = false;
	const ImRect rect(pos1, pos2);
	const ImGuiID id = ImGui::GetCurrentWindow()->GetID((int)edge);
	if (!ImGui::ItemAdd(rect, id)) {
		return false;
	}

	bool hovered = false, held = false;
	/*bool clicked = */ ImGui::ButtonBehavior(rect, id, &hovered, &held);

	ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), colorInt, 0.0f, 0,
										hovered ? 2.0f : 1.0f);
	if (held && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		const glm::ivec2 &pixelPos = image::Image::pixels({u, v}, uiImageSize.x, uiImageSize.y);
		const ImVec2 &mouseDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		const int px = glm::clamp((int)(pixelPos.x + mouseDelta.x), 0, uiImageSize.x - 1);
		const int py = glm::clamp((int)(pixelPos.y - mouseDelta.y), 0, uiImageSize.y - 1);
		const glm::vec2 uv = image::Image::uv(px, py, uiImageSize.x, uiImageSize.y);
		u = uv.x;
		v = uv.y;
		retVal = true;
	}
	return retVal;
}

void BrushPanel::createPopups(command::CommandExecutionListener &listener) {
	const core::String title = makeTitle(_("UV editor"), POPUP_TITLE_UV_EDITOR);
	bool showUVEditor = true;
	if (ImGui::BeginPopupModal(title.c_str(), &showUVEditor, ImGuiWindowFlags_AlwaysAutoResize)) {
		{
			ui::ScopedStyle style;
			style.pushFontSize(imguiApp()->bigFontSize());
			ui::Toolbar toolbar("toolbar", &listener);
			toolbar.button(ICON_LC_FLIP_HORIZONTAL, "texturebrushmirroru");
			toolbar.button(ICON_LC_FLIP_VERTICAL, "texturebrushmirrorv");
			toolbar.button(ICON_LC_X, "texturebrushresetuv");
		}

		const glm::ivec2 cursor = ImGui::GetCursorScreenPos();
		Modifier &modifier = _sceneMgr->modifier();
		TextureBrush &brush = modifier.textureBrush();
		const image::ImagePtr &image = brush.image();

		glm::vec2 uv0 = brush.uv0();
		glm::vec2 uv1 = brush.uv1();

		const video::TexturePtr &texture = _texturePool->load(image->name());
		const float w = ImGui::Size(70);
		const float stretchFactor = w / image->width();
		const float h = image->height() * stretchFactor;
		const ImVec2 uiImageSize(w, h);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("#texturebrushimage", uiImageSize);
		ImGui::AddImage(texture->handle());
		const glm::ivec2 pixelMins =
			cursor + image::Image::pixels(uv0, w, h, image::TextureWrap::Repeat, image::TextureWrap::Repeat, true);
		const glm::ivec2 pixelMaxs =
			cursor + image::Image::pixels(uv1, w, h, image::TextureWrap::Repeat, image::TextureWrap::Repeat, true);
		const glm::vec4 color = _app->color(style::StyleColor::ColorUVEditor) * 255.0f;
		const uint32_t colorInt = IM_COL32(color.r, color.g, color.b, color.a);

		bool dirty = false;
		ImGui::GetWindowDrawList()->AddRect(pixelMins, pixelMaxs, colorInt, 0.0f, 0, 1.0f);
		if (addUVHandle(UVEdge::UpperLeft, pixelMins, pixelMaxs, uiImageSize, colorInt, uv0.x, uv0.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::LowerRight, pixelMins, pixelMaxs, uiImageSize, colorInt, uv1.x, uv1.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::UpperRight, pixelMins, pixelMaxs, uiImageSize, colorInt, uv1.x, uv0.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::LowerLeft, pixelMins, pixelMaxs, uiImageSize, colorInt, uv0.x, uv1.y)) {
			dirty = true;
		}
		if (dirty) {
			brush.setUV0(uv0);
			brush.setUV1(uv1);
		}

		ImGui::EndPopup();
	}
}

void BrushPanel::executeExtrudeBrush() {
	Modifier &modifier = _sceneMgr->modifier();
	if (!modifier.beginBrushFromPanel()) {
		return;
	}
	_sceneMgr->nodeForeachGroup([&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			auto callback = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				_sceneMgr->modified(nodeId, region, flags);
			};
			modifier.execute(_sceneMgr->sceneGraph(), *node, callback);
		}
	});
	modifier.endBrush();
	modifier.extrudeBrush().markDirty();
}

void BrushPanel::updateExtrudeBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	ExtrudeBrush &brush = modifier.extrudeBrush();

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);

	// Fallback used before a node is loaded; overridden by actual node dimensions below.
	static constexpr int DefaultMaxExtrudeDepth = 128;
	int maxDepth = DefaultMaxExtrudeDepth;
	if (node && node->volume()) {
		const voxel::Region &r = node->volume()->region();
		maxDepth = glm::max(r.getWidthInVoxels(), glm::max(r.getHeightInVoxels(), r.getDepthInVoxels()));
	}

	const voxel::FaceNames extrudeFace = brush.face();

	const glm::vec4 &warningTextColor = style::color(style::StyleColor::ColorWarningText);
	if (extrudeFace == voxel::FaceNames::Max) {
		ImGui::TextColored(warningTextColor, "%s", _("Click a voxel face in the viewport to set the extrusion direction"));
		return;
	}

	ImGui::Text(_("Direction: %s"), voxel::faceNameString(extrudeFace));

	int depth = brush.depth();
	if (depth == 0 && (!node || !node->hasSelection())) {
		ImGui::TextColored(warningTextColor, "%s", _("No selection active - use the Select brush first"));
	}
	// All depth/offset changes are preview-only. The preview system creates a fresh
	// volume copy each frame and generate() writes to it. Commit happens on brush switch.
	ImGui::TextUnformatted(_("Depth"));
	if (ImGui::Button("-##extrude_depth")) {
		brush.setDepth(depth - 1);
	}
	ImGui::SameLine();
	if (ImGui::SliderInt("##extrude_depth_slider", &depth, -maxDepth, maxDepth)) {
		brush.setDepth(depth);
	}
	ImGui::SameLine();
	if (ImGui::Button("+##extrude_depth")) {
		brush.setDepth(depth + 1);
	}

	bool fillWalls = brush.fillWalls();
	if (ImGui::Checkbox(_("Fill walls"), &fillWalls)) {
		brush.setFillWalls(fillWalls);
	}

	// Lateral offset sliders - only shown when depth is non-zero.
	if (depth != 0) {
		static constexpr int NumAxes = 3;
		static constexpr const char *AxisLabels[] = {"X", "Y", "Z"};
		const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(extrudeFace));
		const int perp1 = (axisIdx + 1) % NumAxes;
		const int perp2 = (axisIdx + 2) % NumAxes;

		int offsetU = brush.offsetU();
		ImGui::Text(_("Offset %s"), AxisLabels[perp1]);
		if (ImGui::Button("-##extrude_offsetU")) {
			brush.setOffsetU(offsetU - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##extrude_offsetU_slider", &offsetU, -maxDepth, maxDepth)) {
			brush.setOffsetU(offsetU);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##extrude_offsetU")) {
			brush.setOffsetU(offsetU + 1);
		}

		int offsetV = brush.offsetV();
		ImGui::Text(_("Offset %s"), AxisLabels[perp2]);
		if (ImGui::Button("-##extrude_offsetV")) {
			brush.setOffsetV(offsetV - 1);
		}
		ImGui::SameLine();
		if (ImGui::SliderInt("##extrude_offsetV_slider", &offsetV, -maxDepth, maxDepth)) {
			brush.setOffsetV(offsetV);
		}
		ImGui::SameLine();
		if (ImGui::Button("+##extrude_offsetV")) {
			brush.setOffsetV(offsetV + 1);
		}
	}
}

void BrushPanel::executeTransformBrush() {
	Modifier &modifier = _sceneMgr->modifier();
	if (!modifier.beginBrushFromPanel()) {
		return;
	}
	auto func = [&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			auto callback = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				_sceneMgr->modified(nodeId, region, flags);
			};
			modifier.execute(_sceneMgr->sceneGraph(), *node, callback);
		}
	};
	_sceneMgr->nodeForeachGroup(func);
	modifier.endBrush();
}

void BrushPanel::updateTransformBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	TransformBrush &brush = modifier.transformBrush();

	const int nodeId = _sceneMgr->sceneGraph().activeNode();
	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
	if (!node || !node->hasSelection()) {
		ImGui::TextWrappedUnformatted(_("No selection active - use the Select brush first"));
		return;
	}

	const TransformMode currentTransformMode = brush.transformMode();
	if (ImGui::BeginCombo(_("Transform mode"), _(TransformModeStr[(int)currentTransformMode]), ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)TransformMode::Max; ++i) {
			const TransformMode mode = (TransformMode)i;
			const bool selected = mode == currentTransformMode;
			if (ImGui::Selectable(_(TransformModeStr[i]), selected)) {
				if (_transformDirty || brush.hasSnapshot()) {
					executeTransformBrush();
					_transformDirty = false;
				}
				brush.setTransformMode(mode);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	switch (brush.transformMode()) {
	case TransformMode::Move: {
		_transformMoveOffset = brush.moveOffset();
		if (ImGui::AxisSliders(_transformMoveOffset, -TransformBrush::MaxMoveOffset, TransformBrush::MaxMoveOffset)) {
			brush.setMoveOffset(_transformMoveOffset);
			executeTransformBrush();
		}
		break;
	}

	case TransformMode::Shear: {
		_transformShearOffset = brush.shearOffset();
		if (ImGui::AxisSliders(_transformShearOffset, -TransformBrush::MaxShearOffset, TransformBrush::MaxShearOffset)) {
			brush.setShearOffset(_transformShearOffset);
			executeTransformBrush();
		}
		break;
	}

	case TransformMode::Scale: {
		_transformScale = brush.scale();
		ImGui::Checkbox(_("Uniform"), &_transformUniformScale);
		bool scaleChanged = false;
		if (_transformUniformScale) {
			float uniform = _transformScale.x;
			ImGui::SetNextItemWidth(-1);
			if (ImGui::SliderFloat("##uniform_scale", &uniform, 0.01f, 4.0f, "%.2f")) {
				_transformScale = glm::vec3(uniform);
				scaleChanged = true;
			}
		} else {
			scaleChanged = ImGui::AxisSliders(_transformScale, 0.01f, 4.0f, "%.2f");
		}
		if (scaleChanged) {
			brush.setScale(_transformScale);
			if (brush.snapshotVoxelCount() <= DeferredTransformThreshold) {
				executeTransformBrush();
			} else {
				_transformDirty = true;
			}
		}
		break;
	}

	case TransformMode::Rotate: {
		_transformRotation = brush.rotationDegrees();
		if (ImGui::AxisSliders(_transformRotation, -360.0f, 360.0f, "%.1f")) {
			brush.setRotationDegrees(_transformRotation);
			if (brush.snapshotVoxelCount() <= DeferredTransformThreshold) {
				executeTransformBrush();
			} else {
				_transformDirty = true;
			}
		}
		break;
	}

	default:
		break;
	}

	if (brush.transformMode() == TransformMode::Scale || brush.transformMode() == TransformMode::Rotate) {
		if (_transformDirty) {
			if (ImGui::ButtonFullWidth(_("Apply"))) {
				executeTransformBrush();
				_transformDirty = false;
			}
		}
		int samplingInt = (int)brush.voxelSampling();
		if (ImGui::BeginCombo(_("Sampling"), _(VoxelSamplingStr[samplingInt]))) {
			for (int i = 0; i < (int)voxel::VoxelSampling::Max; ++i) {
				const bool selected = samplingInt == i;
				if (ImGui::Selectable(_(VoxelSamplingStr[i]), selected)) {
					brush.setVoxelSampling((voxel::VoxelSampling)i);
					_transformDirty = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
}

void BrushPanel::executeSculptBrush() {
	Modifier &modifier = _sceneMgr->modifier();
	if (!modifier.beginBrushFromPanel()) {
		return;
	}
	auto func = [&](int nodeId) {
		if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
			if (!node->visible()) {
				return;
			}
			auto callback = [&](const voxel::Region &region, ModifierType type, SceneModifiedFlags flags) {
				_sceneMgr->modified(nodeId, region, flags);
			};
			modifier.execute(_sceneMgr->sceneGraph(), *node, callback);
		}
	};
	_sceneMgr->nodeForeachGroup(func);
	modifier.endBrush();
}

void BrushPanel::updateSculptBrushPanel(command::CommandExecutionListener &listener) {
	Modifier &modifier = _sceneMgr->modifier();
	SculptBrush &brush = modifier.sculptBrush();

	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
	if (node == nullptr || (!node->hasSelection() && !brush.hasSnapshot())) {
		ImGui::TextWrappedUnformatted(_("Select voxels first, then switch to sculpt"));
		return;
	}

	const SculptMode currentMode = brush.sculptMode();
	if (ImGui::BeginCombo(_("Sculpt mode"), _(SculptModeStr[(int)currentMode]), ImGuiComboFlags_None)) {
		for (int i = 0; i < (int)SculptMode::Max; ++i) {
			const SculptMode mode = (SculptMode)i;
			const bool selected = mode == currentMode;
			if (ImGui::Selectable(_(SculptModeStr[i]), selected)) {
				brush.setSculptMode(mode);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	const bool needsFace = currentMode == SculptMode::Flatten || currentMode == SculptMode::SmoothAdditive || currentMode == SculptMode::SmoothErode;
	if (needsFace) {
		const voxel::FaceNames flattenFace = brush.flattenFace();
		if (flattenFace == voxel::FaceNames::Max) {
			const glm::vec4 &warningTextColor = style::color(style::StyleColor::ColorWarningText);
			ImGui::TextColored(warningTextColor, "%s", _("Click a voxel face in the viewport to set the direction"));
			return;
		}
		ImGui::Text(_("Direction: %s"), voxel::faceNameString(flattenFace));
	}

	if (currentMode == SculptMode::SmoothAdditive) {
		int threshold = brush.heightThreshold();
		if (ImGui::SliderInt(_("Height threshold"), &threshold, 1, SculptBrush::MaxHeightThreshold)) {
			brush.setHeightThreshold(threshold);
			executeSculptBrush();
		}
	} else if (currentMode == SculptMode::SmoothErode) {
		bool preserve = brush.preserveTopHeight();
		if (ImGui::Checkbox(_("Preserve top height"), &preserve)) {
			brush.setPreserveTopHeight(preserve);
			executeSculptBrush();
		}
		if (preserve) {
			int trimPerStep = brush.trimPerStep();
			ImGui::TextUnformatted(_("Trim per step"));
			if (ImGui::Button("-##sculpt_trim")) {
				brush.setTrimPerStep(trimPerStep - 1);
				executeSculptBrush();
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##sculpt_trim_slider", &trimPerStep, 1, SculptBrush::MaxTrimPerStep)) {
				brush.setTrimPerStep(trimPerStep);
				executeSculptBrush();
			}
			ImGui::SameLine();
			if (ImGui::Button("+##sculpt_trim")) {
				brush.setTrimPerStep(trimPerStep + 1);
				executeSculptBrush();
			}
		}
	} else if (currentMode != SculptMode::Flatten) {
		float strength = brush.strength();
		if (ImGui::SliderFloat(_("Strength"), &strength, 0.0f, 1.0f)) {
			brush.setStrength(strength);
			executeSculptBrush();
		}
	}

	{
		const int maxIter = needsFace ? SculptBrush::MaxFlattenIterations : SculptBrush::MaxIterations;
		int iterations = brush.iterations();
		if (needsFace) {
			ImGui::TextUnformatted(_("Iterations"));
			if (ImGui::Button("-##sculpt_iter")) {
				brush.setIterations(iterations - 1);
				executeSculptBrush();
			}
			ImGui::SameLine();
			if (ImGui::SliderInt("##sculpt_iter_slider", &iterations, 1, maxIter)) {
				brush.setIterations(iterations);
				executeSculptBrush();
			}
			ImGui::SameLine();
			if (ImGui::Button("+##sculpt_iter")) {
				brush.setIterations(iterations + 1);
				executeSculptBrush();
			}
		} else {
			if (ImGui::SliderInt(_("Iterations"), &iterations, 1, maxIter)) {
				brush.setIterations(iterations);
				executeSculptBrush();
			}
		}
	}
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
		} else if (brushType == BrushType::Select) {
			updateSelectBrushPanel(listener);
		} else if (brushType == BrushType::Texture) {
			updateTextureBrushPanel(listener);
		} else if (brushType == BrushType::Normal) {
			updateNormalBrushPanel(listener);
		} else if (brushType == BrushType::Extrude) {
			updateExtrudeBrushPanel(listener);
		} else if (brushType == BrushType::Transform) {
			updateTransformBrushPanel(listener);
		} else if (brushType == BrushType::Sculpt) {
			updateSculptBrushPanel(listener);
		}
	}

	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrappedUnformatted(_("Click on a voxel to pick the color"));
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());

	voxedit::Modifier &modifier = _sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();
	const bool normalPaletteMode = viewModeNormalPalette(_viewMode->intVal());

	ui::Toolbar toolbarBrush("brushes", &listener);
	for (int i = 0; i < (int)BrushType::Max; ++i) {
		if (i == (int)BrushType::Normal && !normalPaletteMode) {
			continue;
		}
		core::String cmd = core::String::format("brush%s", BrushTypeStr[i]).toLower();
		auto func = [&listener, cmd]() { command::executeCommands(cmd, &listener); };
		core::String tooltip = command::help(cmd);
		if (tooltip.empty()) {
			tooltip = BrushTypeStr[i];
		}
		const bool currentBrush = (int)brushType == i;
		ui::ScopedStyle styleButton;
		if (currentBrush) {
			styleButton.setButtonColor(style::color(style::ColorActiveBrush));
		}
		toolbarBrush.button(BrushTypeIcons[i], tooltip.c_str(), func, !currentBrush);
	}
	toolbarBrush.end();

	ImGui::Separator();

	const ModifierType supported = modifier.checkModifierType();
	if (core::countSetBits(core::enumVal(supported)) > 1) {
		ui::Toolbar toolbarModifiers("modifiers", &listener);
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
	} else {
		modifier.setModifierType(supported);
	}
}

void BrushPanel::update(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanel);
	const core::String title = makeTitle(ICON_LC_BRUSH, _("Brush"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			ImGui::TextWrappedUnformatted(
				_("Brushes are only available in edit mode - you are currently in scene mode"));
		} else {
			addModifiers(listener);
			brushSettings(listener);
			createPopups(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
