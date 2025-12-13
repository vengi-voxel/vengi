/**
 * @file
 */

#include "PalettePanel.h"
#include "color/Color.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/PaletteView.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Voxel.h"
#include "voxelui/DragAndDropPayload.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

PalettePanel::PalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, palette::PaletteCache &paletteCache)
	: Super(app, "palette"), _redColor(ImGui::GetColorU32(color::Red())),
	  _yellowColor(ImGui::GetColorU32(color::Yellow())),
	  _darkRedColor(ImGui::GetColorU32(color::DarkRed())), _paletteCache(paletteCache),
	  _sceneMgr(sceneMgr) {
	_currentSelectedPalette = palette::Palette::getDefaultPaletteName();
}

void PalettePanel::reloadAvailablePalettes() {
	_paletteCache.clear();
	_paletteCache.detectPalettes(true);
}

// only re-order the palette entries without changing the colors for the voxels
static bool dragAndDropSortColors() {
	return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
}

void PalettePanel::handleContextMenu(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
									 command::CommandExecutionListener &listener, palette::Palette &palette) {
	const char buf[2] = {(char)paletteColorIdx, '\0'};
	if (ImGui::BeginPopupContextItem(buf)) {
		if (showColorPicker(paletteColorIdx, node, listener)) {
			_colorPickerChange = true;
		} else if (_colorPickerChange) {
			_colorPickerChange = false;
			_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
		}

		const color::RGBA color = palette.color(paletteColorIdx);
		const bool usableColor = color.a > 0;
		const bool singleSelection = _selectedIndices.size() == 1;
		// we might open the context menu for a color that is not in the selection
		const bool isCurrentInSelection = _selectedIndices.has(paletteColorIdx);
		if (usableColor) {
			for (int i = 0; i < palette::MaterialProperty::MaterialMax; ++i) {
				if (i == palette::MaterialProperty::MaterialNone) {
					continue;
				}
				const palette::MaterialProperty prop = (palette::MaterialProperty)i;
				float value = palette.material(paletteColorIdx).value(prop);
				if (ImGui::SliderFloat(palette::MaterialPropertyName(prop), &value,
									palette::MaterialPropertyMinMax(prop).minVal,
									palette::MaterialPropertyMinMax(prop).maxVal)) {
					memento::ScopedMementoGroup group(_sceneMgr->mementoHandler(), "changematerial");
					if (isCurrentInSelection) {
						for (const auto &e : _selectedIndices) {
							_sceneMgr->nodeSetMaterial(node.id(), e->key, prop, value);
						}
					} else {
						_sceneMgr->nodeSetMaterial(node.id(), paletteColorIdx, prop, value);
					}
				}
			}

			if (color.a != 255) {
				if (ImGui::IconMenuItem(ICON_LC_ERASER, _("Remove alpha"))) {
					memento::ScopedMementoGroup group(_sceneMgr->mementoHandler(), "removealpha");
					if (isCurrentInSelection) {
						for (const auto &e : _selectedIndices) {
							_sceneMgr->nodeRemoveAlpha(node.id(), e->first);
						}
					} else {
						_sceneMgr->nodeRemoveAlpha(node.id(), paletteColorIdx);
					}
				}
			}
			if (singleSelection) {
				// TODO: PALETTE: allow to extract multiple colors to a new node
				const core::String &modelFromColorCmd = core::String::format("colortomodel %i", paletteColorIdx);
				ImGui::CommandIconMenuItem(ICON_LC_UNGROUP, _("Model from color"), modelFromColorCmd.c_str(), true,
										&listener);
				if (palette.hasFreeSlot()) {
					if (ImGui::IconMenuItem(ICON_LC_COPY_PLUS, _("Duplicate color"))) {
						_sceneMgr->nodeDuplicateColor(node.id(), paletteColorIdx);
					}
				}
				if (ImGui::IconMenuItem(ICON_LC_COPY_MINUS, _("Remove color"))) {
					_sceneMgr->nodeRemoveColor(node.id(), paletteColorIdx);
					_selectedIndices.remove(paletteColorIdx);
					_selectedIndicesLast = -1;
				}
			} else {
				if (ImGui::IconMenuItem(ICON_LC_COPY_MINUS, _("Reduce to selected"))) {
					core::Buffer<uint8_t> srcIndiced;
					srcIndiced.reserve(_selectedIndices.size());
					for (const auto &e : _selectedIndices) {
						if (e->key == paletteColorIdx) {
							continue;
						}
						srcIndiced.push_back(e->key);
					}
					_sceneMgr->nodeReduceColors(node.id(), srcIndiced, paletteColorIdx);
				}
			}
		}

		core::String name = palette.colorName(paletteColorIdx);
		if (ImGui::InputText(_("Name"), &name)) {
			palette.setColorName(paletteColorIdx, name);
			_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
		}

		ImGui::EndPopup();
	}
}

void PalettePanel::handleDragAndDrop(uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
									 palette::Palette &palette) {
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
			const uint8_t dragPalIdx = *(const uint8_t *)payload->Data;
			palette::PaletteView &palView = palette.view();
			if (dragAndDropSortColors()) {
				palView.exchangeUIIndices(paletteColorIdx, dragPalIdx);
			} else {
				palette.exchange(paletteColorIdx, palView.uiIndex(dragPalIdx));
			}
			_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::RGBAPayload)) {
			const glm::vec4 color = *(const glm::vec4 *)payload->Data;
			_sceneMgr->nodeSetColor(node.id(), paletteColorIdx, color::getRGBA(color));
		}

		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ImagePayload)) {
			const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
			_importPalette = image->name();
		}
		ImGui::EndDragDropTarget();
	}
}

void PalettePanel::addColor(float startingPosX, uint8_t paletteColorIdx, float colorButtonSize,
							scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	palette::Palette &palette = node.palette();
	const int maxPaletteEntries = palette.colorCount();
	const float borderWidth = 1.0f;
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	ImVec2 globalCursorPos = ImGui::GetCursorScreenPos();
	const ImVec2 v1(globalCursorPos.x + borderWidth, globalCursorPos.y + borderWidth);
	const ImVec2 v2(globalCursorPos.x + colorButtonSize, globalCursorPos.y + colorButtonSize);
	const color::RGBA color = palette.color(paletteColorIdx);
	const bool existingColor = paletteColorIdx < maxPaletteEntries;

	if (existingColor) {
		if (color.a != 255) {
			color::RGBA own = color;
			own.a = 127;
			color::RGBA other = color;
			other.a = 255;
			drawList->AddRectFilledMultiColor(v1, v2, own, own, own, other);
		} else {
			drawList->AddRectFilled(v1, v2, color);
		}
	} else {
		drawList->AddRect(v1, v2, color::RGBA(0, 0, 0, 255));
	}

	const bool usableColor = color.a > 0;
	ImGui::PushID(paletteColorIdx);
	if (ImGui::InvisibleButton("", colorButtonSize)) {
		if (usableColor) {
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
				if (!_selectedIndices.remove(paletteColorIdx)) {
					_selectedIndices.insert(paletteColorIdx);
				}
				_selectedIndicesLast = paletteColorIdx;
			} else if (ImGui::IsKeyDown(ImGuiMod_Shift) && _selectedIndicesLast != -1) {
				const int start = core_min(_selectedIndicesLast, paletteColorIdx);
				const int end = core_max(_selectedIndicesLast, paletteColorIdx);
				for (int i = start; i <= end; ++i) {
					if (palette.color(i).a > 0) {
						_selectedIndices.insert(i);
					}
				}
			} else {
				_selectedIndicesLast = paletteColorIdx;
				_selectedIndices.clear();
				_selectedIndices.insert(paletteColorIdx);
				_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(palette, paletteColorIdx));
			}
		}
	}
	ImGui::PopID();

	if (usableColor) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			const float size = 20;
			const ImVec2 rectMins = ImGui::GetCursorScreenPos();
			const ImVec2 rectMaxs(rectMins.x + size, rectMins.y + size);
			ImGui::GetWindowDrawList()->AddRectFilled(rectMins, rectMaxs, ImGui::GetColorU32(color));
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size + 5);
			if (dragAndDropSortColors()) {
				ImGui::TextUnformatted(_("Release CTRL to change the voxel color"));
			} else {
				ImGui::TextUnformatted(_("Press CTRL to re-order"));
			}

			ImGui::SetDragDropPayload(voxelui::dragdrop::PaletteIndexPayload, (const void *)&paletteColorIdx, sizeof(uint8_t),
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
				_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
			}
		}
	} else if (paletteColorIdx == currentSceneColor()) {
		if (color.a > 0) {
			drawList->AddRect(v1, v2, _yellowColor, 0.0f, 0, 2.0f);
		}
	} else if (paletteColorIdx == currentPaletteColorIndex()) {
		drawList->AddRect(v1, v2, _darkRedColor, 0.0f, 0, 4.0f);
	} else if (_selectedIndices.has(paletteColorIdx)) {
		drawList->AddRect(v1, v2, _darkRedColor, 0.0f, 0, 2.0f);
	}

	if (!palette.colorName(paletteColorIdx).empty()) {
		const int size = colorButtonSize / 3;
		const ImVec2 t1(v2.x - borderWidth, v1.y + borderWidth);
		const ImVec2 t2(t1.x - size, t1.y);
		const ImVec2 t3(t1.x, t1.y + size);
		const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
		drawList->AddTriangleFilled(t1, t2, t3, col);
	}

	globalCursorPos.x += colorButtonSize;
	const float windowPosX = ImGui::GetWindowPos().x;
	const float availableX = ImGui::GetContentRegionAvail().x;
	const float contentRegionWidth = availableX + ImGui::GetCursorPosX();
	if (globalCursorPos.x > windowPosX + contentRegionWidth - colorButtonSize) {
		globalCursorPos.x = startingPosX;
		globalCursorPos.y += colorButtonSize;
	}
	ImGui::SetCursorScreenPos(globalCursorPos);
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
			for (const core::String &palette : _paletteCache.availablePalettes()) {
				if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
					_currentSelectedPalette = palette;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::TooltipTextUnformatted(_("To add your own palettes here, put a palette-name.png into one of\n"
										"the search directories or load it into any node to appear here."));

		ImGui::Checkbox(_("Color match"), &_searchFittingColors);
		ImGui::TooltipTextUnformatted(_("Adopt the current voxels to the best fitting colors of\nthe new palette."));

		if (ImGui::OkButton()) {
			_sceneMgr->loadPalette(_currentSelectedPalette, _searchFittingColors, false);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void PalettePanel::onNewPaletteImport(const core::String& paletteName, bool setActive, bool searchBestColors) {
	if (!setActive) {
		reloadAvailablePalettes();
		_popupSwitchPalette = true;
		_currentSelectedPalette = paletteName;
	}
}

void PalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	palette::Palette &palette = node.palette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_PALETTE, _("File"))) {
			ImGui::CommandIconMenuItem(ICON_LC_PALETTE, _("Import"), "importpalette", true, &listener);
			if (ImGui::IconMenuItem(ICON_LC_PAINTBRUSH, _("Switch"))) {
				reloadAvailablePalettes();
				_popupSwitchPalette = true;
			}
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Export"))) {
				_app->saveDialog(
					[&](const core::String &file, const io::FormatDescription *desc) { palette.save(file.c_str()); },
					{}, palette::palettes(), "palette.png");
			}
			if (ImGui::BeginIconMenu(ICON_LC_DOWNLOAD, _("Lospec"))) {
				const char *command = "loadpalette";
				const core::String &keybinding = _app->getKeyBindingsString(command);
				ImGui::InputText(_("ID"), &_lospecID);
				if (ImGui::IconMenuItem(ICON_LC_CHECK, _("Ok"), keybinding.c_str(), false, true)) {
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
			ImGui::CommandIconMenuItem(ICON_LC_ARCHIVE_RESTORE, _("Original"), "palette_sort original", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_ARROW_DOWN_0_1, _("Hue"), "palette_sort hue", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_ARROW_DOWN_0_1, _("Saturation"), "palette_sort saturation", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SUN, _("Brightness"), "palette_sort brightness", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_ARROW_DOWN_0_1, _("CIELab"), "palette_sort cielab", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(_("Tools"))) {
			ImGui::CommandIconMenuItem(ICON_LC_TRASH, _("Remove unused color"), "palette_removeunused", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_TRASH_2, _("Remove and re-create palette"), "palette_removeunused true", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_PICKAXE, _("Model from color"), "colortomodel", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_RULER_DIMENSION_LINE, _("Contrast stretching"), "palette_contraststretching", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SCALE, _("White balancing"), "palette_whitebalancing", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_REPLACE_ALL, _("Apply to all nodes"), "palette_applyall", true, &listener);
			if (ImGui::BeginIconMenu(ICON_LC_LIGHTBULB, _("Intensity"))) {
				ImGui::SliderFloat("##intensity", &_intensityChange, -1.0f, 1.0f);
				const core::String &paletteChangeCmd = core::String::format("palette_changeintensity %f", _intensityChange);
				if (ImGui::CommandMenuItem(_("Apply"), paletteChangeCmd.c_str(), true, &listener)) {
					_intensityChange = 0.0f;
				}
				ImGui::EndMenu();
			}
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
	palette::Palette &palette = node.palette();
	if (ImGui::ColorEdit4(_("Color closest match"), glm::value_ptr(_closestColor),
						  ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoInputs)) {
		const color::RGBA rgba = color::getRGBA(_closestColor);
		_closestMatchPaletteColorIdx = palette.getClosestMatch(rgba);
	}
	ImGui::TooltipTextUnformatted(_("Select a color to find the closest match in the current loaded palette"));
	ImGui::SameLine();
	char buf[256];
	core::String::formatBuf(buf, sizeof(buf), "%i##closestmatchpalpanel", _closestMatchPaletteColorIdx);
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
	const float frameHeight = ImGui::GetFrameHeight();
	const ImVec2 windowSize(10.0f * frameHeight, contentRegionHeight);
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

			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImDrawListFlags backupFlags = drawList->Flags;
			drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;

			for (int palettePanelIdx = 0; palettePanelIdx < palette::PaletteMaxColors; ++palettePanelIdx) {
				const uint8_t paletteColorIdx = palette.view().uiIndex(palettePanelIdx);
				addColor(pos.x, paletteColorIdx, frameHeight, node, listener);
			}

			drawList->Flags = backupFlags;

			ImGui::Dummy(ImVec2(0, frameHeight));
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
			_sceneMgr->mementoHandler().markPaletteChange(_sceneMgr->sceneGraph(), node);
		}
	}

	ImGui::End();

	if (!_importPalette.empty()) {
		if (_sceneMgr->importPalette(_importPalette, true, true)) {
			core::String paletteName(core::string::extractFilename(_importPalette));
			onNewPaletteImport(paletteName, false, false);
		}
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
	glm::vec4 color = color::fromRGBA(palette.color(paletteColorIdx));
	const int maxPaletteEntries = palette.colorCount();
	const bool existingColor = paletteColorIdx < maxPaletteEntries;

	if (ImGui::ColorPicker4(_("Color"), glm::value_ptr(color), flags)) {
		const bool hasAlpha = palette.color(paletteColorIdx).a != 255;
		palette.setColor(paletteColorIdx, color::getRGBA(color));
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
