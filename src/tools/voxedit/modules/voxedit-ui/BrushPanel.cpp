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
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/LineBrush.h"
#include "voxedit-util/modifier/brush/NormalBrush.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxedit-util/modifier/brush/StampBrush.h"
#include "voxedit-util/modifier/brush/TextureBrush.h"
#include "voxel/RawVolume.h"
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
	ICON_LC_IMAGE,		ICON_LC_MOVE_UP_RIGHT};
static_assert(lengthof(BrushTypeIcons) == (int)BrushType::Max, "BrushTypeIcons size mismatch");

void BrushPanel::init() {
	_renderNormals = core::Var::getSafe(cfg::RenderNormals);
	_viewMode = core::Var::getSafe(cfg::VoxEditViewMode);
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
								   C_("SelectMode", "Fuzzy Color"), C_("SelectMode", "Connected")};
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
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_FILE)) {
		_app->openDialog(
			[&](const core::String &filename, const io::FormatDescription *desc) {
				const image::ImagePtr &image = _texturePool->loadImage(filename);
				brush.setImage(image);
			},
			{}, io::format::images());
	}

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
		}
	}

	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrappedUnformatted(_("Click on a voxel to pick the color"));
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());

	voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
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
