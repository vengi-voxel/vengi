/**
 * @file
 */

#include "PalettePanel.h"
#include "DragAndDropPayload.h"
#include "core/StringUtil.h"
#include "imgui.h"
#include "io/FormatDescription.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "core/Color.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsForkAwesome.h"
#include "voxel/Palette.h"
#include "voxel/Voxel.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include <glm/gtc/type_ptr.hpp>

#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define PALETTEACTIONPOPUP "##paletteactionpopup"

namespace voxedit {

PalettePanel::PalettePanel()
	: _redColor(ImGui::GetColorU32(core::Color::Red)), _yellowColor(ImGui::GetColorU32(core::Color::Yellow)),
	  _darkRedColor(ImGui::GetColorU32(core::Color::DarkRed)) {
	_currentSelectedPalette = voxel::Palette::getDefaultPaletteName();
}

void PalettePanel::reloadAvailablePalettes() {
	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list("", entities, "palette-*.png");
	_availablePalettes.clear();
	for (const io::FilesystemEntry& file : entities) {
		if (file.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String& name = voxel::Palette::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
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

void PalettePanel::addColor(float startingPosX, uint8_t palIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = node.palette();
	const int maxPaletteEntries = palette.colorCount();
	const float borderWidth = 1.0f;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImDrawListFlags backupFlags = drawList->Flags;
	drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;

	const float windowWidth = ImGui::GetWindowContentRegionMax().x;
	const ImVec2 colorButtonSize(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
	ImVec2 globalCursorPos = ImGui::GetCursorScreenPos();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	const ImVec2 v1(globalCursorPos.x + borderWidth, globalCursorPos.y + borderWidth);
	const ImVec2 v2(globalCursorPos.x + colorButtonSize.x, globalCursorPos.y + colorButtonSize.y);
	const bool usableColor = palette.color(palIdx).a > 0;
	const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
	const bool existingColor = palIdx < maxPaletteEntries;
	if (existingColor) {
		if (palette.color(palIdx).a != 255) {
			core::RGBA other = palette.color(palIdx);
			other.a = 255;
			drawList->AddRectFilledMultiColor(v1, v2, palette.color(palIdx), palette.color(palIdx), palette.color(palIdx), other);
		} else {
			drawList->AddRectFilled(v1, v2, palette.color(palIdx));
		}
	} else {
		drawList->AddRect(v1, v2, core::RGBA(0, 0, 0, 255));
	}

	const core::String &id = core::string::format("##palitem-%i", palIdx);
	if (ImGui::InvisibleButton(id.c_str(), colorButtonSize)) {
		if (usableColor) {
			sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(palette, palIdx));
		}
	}

	if (usableColor) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text("Color %i", (int)palIdx);
			ImGui::SetDragDropPayload(dragdrop::PaletteIndexPayload, (const void*)&palIdx, sizeof(uint8_t), ImGuiCond_Always);
			static_assert(sizeof(palIdx) == sizeof(uint8_t), "Unexpected palette index size");
			ImGui::EndDragDropSource();
		}
	} else {
		ImGui::TooltipText("Empty color slot");
	}

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
			const uint8_t dragPalIdx = *(const uint8_t*)payload->Data;
			if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
				palette.exchange(palIdx, dragPalIdx);
			} else {
				core::exchange(palette.color(palIdx), palette.color(dragPalIdx));
				if (!existingColor) {
					palette.setSize(palIdx + 1);
				}
				palette.markDirty();
				palette.markSave();
			}
			sceneMgr().mementoHandler().markPaletteChange(node);
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::RGBAPayload)) {
			const glm::vec4 color = *(const glm::vec4 *)payload->Data;
			const bool hasAlpha = palette.color(palIdx).a != 255;
			palette.color(palIdx) = core::Color::getRGBA(color);
			if (!existingColor) {
				palette.setSize(palIdx + 1);
			} else if (hasAlpha && palette.color(palIdx).a == 255) {
				sceneMgr().updateVoxelType(node.id(), palIdx, voxel::VoxelType::Generic);
			} else if (!hasAlpha && palette.color(palIdx).a != 255) {
				sceneMgr().updateVoxelType(node.id(), palIdx, voxel::VoxelType::Transparent);
			}
			palette.markDirty();
			palette.markSave();
			sceneMgr().mementoHandler().markPaletteChange(node);
		}

		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::ImagePayload)) {
			const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
			_importPalette = image->name();
		}
		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		if (showColorPicker(palIdx, node, listener)) {
			_colorPickerChange = true;
		} else if (_colorPickerChange) {
			_colorPickerChange = false;
			sceneMgr().mementoHandler().markPaletteChange(node);
		}

		if (usableColor) {
			const core::String &modelFromColorCmd = core::string::format("colortomodel %i", palIdx);
			ImGui::CommandMenuItem(ICON_FA_OBJECT_UNGROUP " Model from color" PALETTEACTIONPOPUP, modelFromColorCmd.c_str(), true, &listener);
			if (palette.hasGlow(palIdx)) {
				if (ImGui::MenuItem(ICON_FK_SUN_O " Remove Glow")) {
					palette.removeGlow(palIdx);
					palette.markSave();
					sceneMgr().mementoHandler().markPaletteChange(node);
				}
			} else {
				if (ImGui::MenuItem(ICON_FK_SUN " Glow")) {
					palette.setGlow(palIdx);
					palette.markSave();
					sceneMgr().mementoHandler().markPaletteChange(node);
				}
			}
		}

		ImGui::EndPopup();
	}
	if (!_colorHovered && ImGui::IsItemHovered()) {
		_colorHovered = true;
		drawList->AddRect(v1, v2, _redColor, 0.0f, 0, 2.0f);
	} else if (palIdx == currentSceneColor()) {
		if (palette.color(currentSceneColor()).a > 0) {
			drawList->AddRect(v1, v2, _yellowColor, 0.0f, 0, 2.0f);
		}
	} else if (palIdx == currentPaletteIndex()) {
		drawList->AddRect(v1, v2, _darkRedColor, 0.0f, 0, 2.0f);
	}
	globalCursorPos.x += colorButtonSize.x;
	if (globalCursorPos.x > windowPos.x + windowWidth - colorButtonSize.x) {
		globalCursorPos.x = startingPosX;
		globalCursorPos.y += colorButtonSize.y;
	}
	ImGui::SetCursorScreenPos(globalCursorPos);
	// restore the draw list flags from above
	drawList->Flags = backupFlags;
}

uint8_t PalettePanel::currentPaletteIndex() const {
	return sceneMgr().modifier().cursorVoxel().getColor();
}

uint8_t PalettePanel::currentSceneColor() const {
	return sceneMgr().hitCursorVoxel().getColor();
}

void PalettePanel::createPopups(scenegraph::SceneGraphNode &node) {
	if (_popupSwitchPalette) {
		ImGui::OpenPopup(POPUP_TITLE_LOAD_PALETTE);
		_popupSwitchPalette = false;
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_LOAD_PALETTE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Select the palette");
		ImGui::Separator();
		if (ImGui::BeginCombo("##type", _currentSelectedPalette.c_str(), 0)) {
			for (const core::String& palette : _availablePalettes) {
				if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
					_currentSelectedPalette = palette;
				}
			}
			for (int i = 0; i < lengthof(voxel::Palette::builtIn); ++i) {
				if (ImGui::Selectable(voxel::Palette::builtIn[i], voxel::Palette::builtIn[i] == _currentSelectedPalette)) {
					_currentSelectedPalette = voxel::Palette::builtIn[i];
				}
			}
			ImGui::EndCombo();
		}
		ImGui::TooltipText("To add your own palettes here, put a palette-name.png into one of\n"
						   "the search directories or load it into any node to appear here.");

		ImGui::Checkbox("Color match", &_searchFittingColors);
		ImGui::TooltipText("Adopt the current voxels to the best fitting colors of\nthe new palette.");

		if (ImGui::Button(ICON_FA_CHECK " OK##loadpalette")) {
			sceneMgr().loadPalette(_currentSelectedPalette, _searchFittingColors);
			sceneMgr().mementoHandler().markPaletteChange(node);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_XMARK " Cancel##loadpalette")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void PalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = node.palette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu(ICON_FA_PALETTE" File")) {
			ImGui::CommandMenuItem(ICON_FA_PALETTE " Load##loadpalette", "importpalette", true, &listener);
			if (ImGui::MenuItem(ICON_FA_PAINTBRUSH " Switch##switchpalette")) {
				reloadAvailablePalettes();
				_popupSwitchPalette = true;
			}
			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Export##savepalette")) {
				imguiApp()->saveDialog([&](const core::String &file, const io::FormatDescription *desc) { palette.save(file.c_str()); }, {}, io::format::palettes(), "palette.png");
			}
			ImGui::TooltipText("Export the palette");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_SORT " Sort##sortpalette")) {
			ImGui::CommandMenuItem("Hue", "palette_sort hue", true, &listener);
			ImGui::CommandMenuItem("Saturation", "palette_sort saturation", true, &listener);
			ImGui::CommandMenuItem("Brightness", "palette_sort brightness", true, &listener);
			ImGui::CommandMenuItem("CIELab", "palette_sort cielab", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools##toolspalette")) {
			ImGui::CommandMenuItem("Remove unused color", "palette_removeunused", true, &listener);
			// TODO: add color quanitisation to parts of the palette
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void PalettePanel::closestColor(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	ImGui::SliderFloat(ICON_FA_SLIDERS, &_intensityChange, -1.0f, 1.0f);
	ImGui::SameLine();
	const core::String &paletteChangeCmd = core::string::format("palette_changeintensity %f", _intensityChange);
	if (ImGui::CommandButton("Apply", paletteChangeCmd.c_str(), nullptr, ImVec2(0.0f, 0.0f), &listener)) {
		_intensityChange = 0.0f;
	}

	voxel::Palette &palette = node.palette();
	if (ImGui::ColorEdit4("Color closest match", glm::value_ptr(_closestColor), ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoInputs)) {
		const core::RGBA rgba = core::Color::getRGBA(_closestColor);
		_closestMatch = palette.getClosestMatch(rgba);
	}
	ImGui::TooltipText("Select a color to find the closest match in the current loaded palette");
	ImGui::SameLine();
	char buf[256];
	core::string::formatBuf(buf, sizeof(buf), "%i##closestmatchpalpanel", _closestMatch);
	if (ImGui::Selectable(buf) && _closestMatch != -1) {
		sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(palette, _closestMatch));
	}
}

void PalettePanel::update(const char *title, command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	const ImVec2 windowSize(10.0f * ImGui::GetFrameHeight(), ImGui::GetContentRegionMax().y);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const int currentSceneHoveredPalIdx = currentSceneColor();
	const int currentSelectedPalIdx = currentPaletteIndex();
	_hasFocus = false;
	_importPalette.clear();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_MenuBar)) {
		_hasFocus = ImGui::IsWindowHovered();
		_colorHovered = false;

		if (node.type() == scenegraph::SceneGraphNodeType::Model) {
			paletteMenuBar(node, listener);
			const ImVec2 &pos = ImGui::GetCursorScreenPos();
			for (int palIdx = 0; palIdx < voxel::PaletteMaxColors; ++palIdx) {
				addColor(pos.x, palIdx, node, listener);
			}
			ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
			ImGui::Text("palette index: %i (scene voxel index %i)", currentSelectedPalIdx, currentSceneHoveredPalIdx);

			createPopups(node);
			closestColor(node, listener);
		}
	}

	if (core::Var::getSafe(cfg::VoxEditShowColorPicker)->boolVal()) {
		if (showColorPicker(currentSelectedPalIdx, node, listener)) {
			_colorPickerChange = true;
		} else if (_colorPickerChange) {
			_colorPickerChange = false;
			sceneMgr().mementoHandler().markPaletteChange(node);
		}
	}

	ImGui::End();

	if (!_importPalette.empty()) {
		sceneMgr().importPalette(_importPalette);
	}
}

bool PalettePanel::showColorPicker(uint8_t palIdx, scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = node.palette();
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB;
	flags |= ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar;
	if (core::Var::getSafe(cfg::VoxEditColorWheel)->boolVal()) {
		flags |= ImGuiColorEditFlags_PickerHueWheel;
	} else {
		flags |= ImGuiColorEditFlags_PickerHueBar;
	}
	glm::vec4 color = core::Color::fromRGBA(palette.color(palIdx));
	const int maxPaletteEntries = palette.colorCount();
	const bool existingColor = palIdx < maxPaletteEntries;

	if (ImGui::ColorPicker4("Color", glm::value_ptr(color), flags)) {
		const bool hasAlpha = palette.color(palIdx).a != 255;
		palette.color(palIdx) = core::Color::getRGBA(color);
		if (!existingColor) {
			palette.setSize(palIdx + 1);
		} else if (hasAlpha && palette.color(palIdx).a == 255) {
			sceneMgr().updateVoxelType(node.id(), palIdx, voxel::VoxelType::Generic);
		} else if (!hasAlpha && palette.color(palIdx).a != 255) {
			sceneMgr().updateVoxelType(node.id(), palIdx, voxel::VoxelType::Transparent);
		}
		palette.markDirty();
		palette.markSave();
		return true;
	}
	return false;
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

}
