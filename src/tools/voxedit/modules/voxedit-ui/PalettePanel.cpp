/**
 * @file
 */

#include "PalettePanel.h"
#include "DragAndDropPayload.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Voxel.h"
#include <glm/gtc/type_ptr.hpp>

#define PALETTEACTIONPOPUP "##paletteactionpopup"

namespace voxedit {

PalettePanel::PalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
	: Super(app, "palette"), _redColor(ImGui::GetColorU32(core::Color::Red())),
	  _yellowColor(ImGui::GetColorU32(core::Color::Yellow())),
	  _darkRedColor(ImGui::GetColorU32(core::Color::DarkRed())), _sceneMgr(sceneMgr) {
	_currentSelectedPalette = palette::Palette::getDefaultPaletteName();
}

void PalettePanel::reloadAvailablePalettes() {
	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list("", entities, "palette-*.png");
	_availablePalettes.clear();
	for (const io::FilesystemEntry &file : entities) {
		if (file.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String &name = palette::Palette::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		core::String id;
		if (node.name().empty()) {
			id = core::string::format("node:%i##%i", node.id(), node.id());
		} else {
			id = core::string::format("node:%s##%i", node.name().c_str(), node.id());
		}
		_availablePalettes.push_back(id);
	}
}

// only re-order the palette entries without changing the colors for the voxels
static bool dragAndDropSortColors() {
	return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
}

void PalettePanel::handleContextMenu(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
									 command::CommandExecutionListener &listener, palette::Palette &palette) {
	const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", paletteColorIdx);
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		if (showColorPicker(paletteColorIdx, node, listener)) {
			_colorPickerChange = true;
		} else if (_colorPickerChange) {
			_colorPickerChange = false;
			_sceneMgr->mementoHandler().markPaletteChange(node);
		}

		const bool usableColor = palette.color(paletteColorIdx).a > 0;
		if (usableColor) {
			for (int i = 0; i < palette::MaterialProperty::MaterialMax - 1; ++i) {
				if (i == palette::MaterialProperty::MaterialNone) {
					continue;
				}
				const palette::MaterialProperty prop = (palette::MaterialProperty)i;
				float value = palette.material(paletteColorIdx).value(prop);
				if (ImGui::SliderFloat(palette::MaterialPropertyName(prop), &value,
									   palette::MaterialPropertyMinMax(prop).minVal,
									   palette::MaterialPropertyMinMax(prop).maxVal)) {
					_sceneMgr->nodeSetMaterial(node.id(), paletteColorIdx, prop, value);
				}
			}
			const core::String &modelFromColorCmd = core::string::format("colortomodel %i", paletteColorIdx);
			ImGui::CommandIconMenuItem(ICON_LC_UNGROUP, _("Model from color"), modelFromColorCmd.c_str(), true,
									   &listener);

			if (palette.color(paletteColorIdx).a != 255) {
				if (ImGui::IconMenuItem(ICON_LC_ERASER, _("Remove alpha"))) {
					_sceneMgr->nodeRemoveAlpha(node.id(), paletteColorIdx);
				}
			}
			if (palette.hasFreeSlot()) {
				if (ImGui::IconMenuItem(ICON_LC_COPY_PLUS, _("Duplicate color"))) {
					_sceneMgr->nodeDuplicateColor(node.id(), paletteColorIdx);
				}
			}
			if (ImGui::IconMenuItem(ICON_LC_COPY_MINUS, _("Remove color"))) {
				_sceneMgr->nodeRemoveColor(node.id(), paletteColorIdx);
			}
		}

		ImGui::EndPopup();
	}
}

void PalettePanel::handleDragAndDrop(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
									 palette::Palette &palette) {
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
			const uint8_t dragPalIdx = *(const uint8_t *)payload->Data;
			if (dragAndDropSortColors()) {
				palette.exchangeUIIndices(paletteColorIdx, dragPalIdx);
			} else {
				palette.exchange(paletteColorIdx, palette.uiIndex(dragPalIdx));
			}
			_sceneMgr->mementoHandler().markPaletteChange(node);
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::RGBAPayload)) {
			const glm::vec4 color = *(const glm::vec4 *)payload->Data;
			_sceneMgr->nodeSetColor(node.id(), paletteColorIdx, core::Color::getRGBA(color));
		}

		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ImagePayload)) {
			const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
			_importPalette = image->name();
		}
		ImGui::EndDragDropTarget();
	}
}

void PalettePanel::addColor(float startingPosX, uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
							command::CommandExecutionListener &listener) {
	palette::Palette &palette = node.palette();
	const int maxPaletteEntries = palette.colorCount();
	const float borderWidth = 1.0f;
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	const ImDrawListFlags backupFlags = drawList->Flags;
	drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionWidth = available.x + ImGui::GetCursorPosX();
	const ImVec2 colorButtonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
	ImVec2 globalCursorPos = ImGui::GetCursorScreenPos();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	const ImVec2 v1(globalCursorPos.x + borderWidth, globalCursorPos.y + borderWidth);
	const ImVec2 v2(globalCursorPos.x + colorButtonSize.x, globalCursorPos.y + colorButtonSize.y);
	const bool usableColor = palette.color(paletteColorIdx).a > 0;
	const bool existingColor = paletteColorIdx < maxPaletteEntries;
	if (existingColor) {
		const core::RGBA color = palette.color(paletteColorIdx);
		if (color.a != 255) {
			core::RGBA own = color;
			own.a = 127;
			core::RGBA other = color;
			other.a = 255;
			drawList->AddRectFilledMultiColor(v1, v2, own, own, own, other);
		} else {
			drawList->AddRectFilled(v1, v2, color);
		}
	} else {
		drawList->AddRect(v1, v2, core::RGBA(0, 0, 0, 255));
	}

	ImGui::PushID(paletteColorIdx);
	if (ImGui::InvisibleButton("", colorButtonSize)) {
		if (usableColor) {
			_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(palette, paletteColorIdx));
		}
	}
	ImGui::PopID();

	if (usableColor) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			const float size = 20;
			const ImVec2 v1 = ImGui::GetCursorScreenPos();
			const ImVec2 v2(v1.x + size, v1.y + size);
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(paletteColorIdx)));
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size + 5);
			if (dragAndDropSortColors()) {
				ImGui::TextUnformatted(_("Release CTRL to change the voxel color"));
			} else {
				ImGui::TextUnformatted(_("Press CTRL to re-order"));
			}

			ImGui::SetDragDropPayload(dragdrop::PaletteIndexPayload, (const void *)&paletteColorIdx, sizeof(uint8_t),
									  ImGuiCond_Always);
			static_assert(sizeof(paletteColorIdx) == sizeof(uint8_t), "Unexpected palette index size");
			ImGui::EndDragDropSource();
		}
	} else {
		ImGui::TooltipTextUnformatted(_("Empty color slot"));
	}

	handleDragAndDrop(paletteColorIdx, node, palette);

	handleContextMenu(paletteColorIdx, node, listener, palette);

	if (!_colorHovered && ImGui::IsItemHovered()) {
		_colorHovered = true;
		drawList->AddRect(v1, v2, _redColor, 0.0f, 0, 2.0f);

		if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C)) {
			_copyPaletteColorIdx = paletteColorIdx;
		} else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V)) {
			if (_copyPaletteColorIdx != -1) {
				palette.copy(_copyPaletteColorIdx, paletteColorIdx);
				_sceneMgr->mementoHandler().markPaletteChange(node);
			}
		}
	} else if (paletteColorIdx == currentSceneColor()) {
		if (palette.color(paletteColorIdx).a > 0) {
			drawList->AddRect(v1, v2, _yellowColor, 0.0f, 0, 2.0f);
		}
	} else if (paletteColorIdx == currentPaletteColorIndex()) {
		drawList->AddRect(v1, v2, _darkRedColor, 0.0f, 0, 2.0f);
	}
	globalCursorPos.x += colorButtonSize.x;
	if (globalCursorPos.x > windowPos.x + contentRegionWidth - colorButtonSize.x) {
		globalCursorPos.x = startingPosX;
		globalCursorPos.y += colorButtonSize.y;
	}
	ImGui::SetCursorScreenPos(globalCursorPos);
	// restore the draw list flags from above
	drawList->Flags = backupFlags;
}

uint8_t PalettePanel::currentPaletteColorIndex() const {
	return _sceneMgr->modifier().cursorVoxel().getColor();
}

uint8_t PalettePanel::currentSceneColor() const {
	return _sceneMgr->hitCursorVoxel().getColor();
}

void PalettePanel::createPopups(scenegraph::SceneGraphNode &node) {
	if (_popupSwitchPalette) {
		ImGui::OpenPopup(POPUP_TITLE_LOAD_PALETTE);
		_popupSwitchPalette = false;
	}

	const core::String title = makeTitle(_("Select Palette"), POPUP_TITLE_LOAD_PALETTE);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(_("Select the palette"));
		ImGui::Separator();
		if (ImGui::BeginCombo("##type", _currentSelectedPalette.c_str(), 0)) {
			for (const core::String &palette : _availablePalettes) {
				if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
					_currentSelectedPalette = palette;
				}
			}
			for (int i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
				if (ImGui::Selectable(palette::Palette::builtIn[i],
									  palette::Palette::builtIn[i] == _currentSelectedPalette)) {
					_currentSelectedPalette = palette::Palette::builtIn[i];
				}
			}
			ImGui::EndCombo();
		}
		ImGui::TooltipTextUnformatted(_("To add your own palettes here, put a palette-name.png into one of\n"
										"the search directories or load it into any node to appear here."));

		ImGui::Checkbox(_("Color match"), &_searchFittingColors);
		ImGui::TooltipTextUnformatted(_("Adopt the current voxels to the best fitting colors of\nthe new palette."));

		if (ImGui::IconButton(ICON_LC_CHECK, _("OK"))) {
			_sceneMgr->loadPalette(_currentSelectedPalette, _searchFittingColors, false);
			_sceneMgr->mementoHandler().markPaletteChange(node);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_X, _("Cancel"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void PalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	palette::Palette &palette = node.palette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_PALETTE, _("File"))) {
			ImGui::CommandIconMenuItem(ICON_LC_PALETTE, _("Load"), "importpalette", true, &listener);
			if (ImGui::IconMenuItem(ICON_LC_PAINTBRUSH, _("Switch"))) {
				reloadAvailablePalettes();
				_popupSwitchPalette = true;
			}
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Export"))) {
				_app->saveDialog(
					[&](const core::String &file, const io::FormatDescription *desc) { palette.save(file.c_str()); },
					{}, io::format::palettes(), "palette.png");
			}
			if (ImGui::BeginIconMenu(ICON_LC_DOWNLOAD, _("Lospec"))) {
				const char *command = "loadpalette";
				const core::String &keybinding = _app->getKeyBindingsString(command);
				ImGui::InputText(_("ID"), &_lospecID);
				if (ImGui::MenuItem(_("OK"), keybinding.c_str(), false, true)) {
					core::String cmd = command;
					cmd.append(" lospec:");
					cmd.append(_lospecID);
					command::executeCommands(cmd, &listener);
				}
				ImGui::TooltipCommand(command);
				ImGui::EndMenu();
			}
			ImGui::TooltipTextUnformatted(_("Export the palette"));
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_ARROW_DOWN_NARROW_WIDE, _("Sort"))) {
			ImGui::CommandMenuItem(_("Original"), "palette_sort original", true, &listener);
			ImGui::CommandMenuItem(_("Hue"), "palette_sort hue", true, &listener);
			ImGui::CommandMenuItem(_("Saturation"), "palette_sort saturation", true, &listener);
			ImGui::CommandMenuItem(_("Brightness"), "palette_sort brightness", true, &listener);
			ImGui::CommandMenuItem(_("CIELab"), "palette_sort cielab", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(_("Tools"))) {
			ImGui::CommandMenuItem(_("Remove unused color"), "palette_removeunused", true, &listener);
			ImGui::CommandMenuItem(_("Remove and re-create palette"), "palette_removeunused true", true, &listener);
			ImGui::CommandMenuItem(_("Model from color"), "colortomodel", true, &listener);
			// TODO: add color quantisation to parts of the palette
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
			ImGui::CheckboxVar(_("Color picker"), cfg::VoxEditShowColorPicker);
			ImGui::CheckboxVar(_("Color wheel"), cfg::VoxEditColorWheel);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void PalettePanel::closestColor(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	ImGui::SliderFloat(ICON_LC_SLIDERS_HORIZONTAL, &_intensityChange, -1.0f, 1.0f);
	ImGui::SameLine();
	const core::String &paletteChangeCmd = core::string::format("palette_changeintensity %f", _intensityChange);
	if (ImGui::CommandButton(_("Apply"), paletteChangeCmd.c_str(), nullptr, ImVec2(0.0f, 0.0f), &listener)) {
		_intensityChange = 0.0f;
	}

	palette::Palette &palette = node.palette();
	if (ImGui::ColorEdit4(_("Color closest match"), glm::value_ptr(_closestColor),
						  ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoInputs)) {
		const core::RGBA rgba = core::Color::getRGBA(_closestColor);
		_closestMatchPaletteColorIdx = palette.getClosestMatch(rgba);
	}
	ImGui::TooltipTextUnformatted(_("Select a color to find the closest match in the current loaded palette"));
	ImGui::SameLine();
	char buf[256];
	core::string::formatBuf(buf, sizeof(buf), "%i##closestmatchpalpanel", _closestMatchPaletteColorIdx);
	if (ImGui::Selectable(buf) && _closestMatchPaletteColorIdx != -1) {
		_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(palette, _closestMatchPaletteColorIdx));
	}
}

void PalettePanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(PalettePanel);
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionHeight = available.y + ImGui::GetCursorPosY();
	const ImVec2 windowSize(10.0f * ImGui::GetFrameHeight(), contentRegionHeight);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const int sceneHoveredPaletteColorIdx = currentSceneColor();
	const int selectedPaletteColorIdx = currentPaletteColorIndex();
	_hasFocus = false;
	_importPalette.clear();
	const core::String title = makeTitle(ICON_LC_PALETTE, _("Palette"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
		_hasFocus = ImGui::IsWindowHovered();
		_colorHovered = false;

		if (node.isModelNode()) {
			paletteMenuBar(node, listener);
			const ImVec2 &pos = ImGui::GetCursorScreenPos();
			const palette::Palette &palette = node.palette();
			for (int palettePanelIdx = 0; palettePanelIdx < palette::PaletteMaxColors; ++palettePanelIdx) {
				const uint8_t paletteColorIdx = palette.uiIndex(palettePanelIdx);
				addColor(pos.x, paletteColorIdx, node, listener);
			}
			ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
			ImGui::Text(_("Palette index: %i (scene voxel index %i)"), selectedPaletteColorIdx,
						sceneHoveredPaletteColorIdx);

			createPopups(node);
			closestColor(node, listener);
		}
	}

	if (core::Var::getSafe(cfg::VoxEditShowColorPicker)->boolVal()) {
		if (showColorPicker(selectedPaletteColorIdx, node, listener)) {
			_colorPickerChange = true;
		} else if (_colorPickerChange) {
			_colorPickerChange = false;
			_sceneMgr->mementoHandler().markPaletteChange(node);
		}
	}

	ImGui::End();

	if (!_importPalette.empty()) {
		_sceneMgr->importPalette(_importPalette, false);
	}
}

bool PalettePanel::showColorPicker(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
								   command::CommandExecutionListener &listener) {
	palette::Palette &palette = node.palette();
	ImGuiColorEditFlags flags =
		ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB;
	flags |= ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar;
	if (core::Var::getSafe(cfg::VoxEditColorWheel)->boolVal()) {
		flags |= ImGuiColorEditFlags_PickerHueWheel;
	} else {
		flags |= ImGuiColorEditFlags_PickerHueBar;
	}
	glm::vec4 color = core::Color::fromRGBA(palette.color(paletteColorIdx));
	const int maxPaletteEntries = palette.colorCount();
	const bool existingColor = paletteColorIdx < maxPaletteEntries;

	if (ImGui::ColorPicker4(_("Color"), glm::value_ptr(color), flags)) {
		const bool hasAlpha = palette.color(paletteColorIdx).a != 255;
		palette.setColor(paletteColorIdx, core::Color::getRGBA(color));
		if (existingColor) {
			if (hasAlpha && palette.color(paletteColorIdx).a == 255) {
				_sceneMgr->nodeUpdateVoxelType(node.id(), paletteColorIdx, voxel::VoxelType::Generic);
			} else if (!hasAlpha && palette.color(paletteColorIdx).a != 255) {
				_sceneMgr->nodeUpdateVoxelType(node.id(), paletteColorIdx, voxel::VoxelType::Transparent);
			}
			_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(palette, paletteColorIdx));
		}
		palette.markSave();
		return true;
	}
	return false;
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

} // namespace voxedit
