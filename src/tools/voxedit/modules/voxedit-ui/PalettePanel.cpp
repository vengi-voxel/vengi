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
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
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
	if (entities.empty()) {
		Log::error("Could not find any palettes");
	}
	_availablePalettes.clear();
	for (const io::FilesystemEntry& file : entities) {
		if (file.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String& name = voxel::Palette::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
	for (const voxelformat::SceneGraphNode &node : sceneMgr().sceneGraph()) {
		core::String id;
		if (node.name().empty()) {
			id = core::string::format("node:%i##%i", node.id(), node.id());
		} else {
			id = core::string::format("node:%s##%i", node.name().c_str(), node.id());
		}
		_availablePalettes.push_back(id);
	}
}

void PalettePanel::addColor(float startingPosX, uint8_t palIdx, voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = node.palette();
	const int maxPaletteEntries = palette.colorCount;
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
	const bool usableColor = palette.colors[palIdx].a > 0;
	const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
	const bool existingColor = palIdx < maxPaletteEntries;
	if (existingColor) {
		drawList->AddRectFilled(v1, v2, palette.colors[palIdx]);
	} else {
		drawList->AddRect(v1, v2, core::RGBA(0, 0, 0, 255));
	}

	const core::String &id = core::string::format("##palitem-%i", palIdx);
	if (ImGui::InvisibleButton(id.c_str(), colorButtonSize)) {
		if (usableColor) {
			voxel::VoxelType type;
			if (palette.colors[palIdx] < 255) {
				type = voxel::VoxelType::Transparent;
			} else {
				type = voxel::VoxelType::Generic;
			}
			sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(type, palIdx));
		}
	}

	if (usableColor) {
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::Text("Color %i", palIdx);
			ImGui::SetDragDropPayload(dragdrop::ColorPayload, (const void*)&palIdx, sizeof(int), ImGuiCond_Always);
			ImGui::EndDragDropSource();
		}
	} else {
		ImGui::TooltipText("Empty color slot");
	}

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::ColorPayload)) {
			const int dragPalIdx = *(int*)payload->Data;
			core::exchange(palette.colors[palIdx], palette.colors[dragPalIdx]);
			if (!existingColor) {
				palette.colorCount = palIdx + 1;
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
		showColorPicker(palIdx, node, listener);

		if (usableColor) {
			const core::String &modelFromColorCmd = core::string::format("colortolayer %i", palIdx);
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
		if (palette.colors[currentSceneColor()].a > 0) {
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

void PalettePanel::createPopups() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_LOAD_PALETTE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Select the palette");
		ImGui::Separator();
		if (ImGui::BeginCombo(ICON_FA_TREE " Type", _currentSelectedPalette.c_str(), 0)) {
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

		ImGui::Checkbox("Color match", &_searchFittingColors);
		ImGui::TooltipText("Search for matching colors in the new palette");

		if (ImGui::Button(ICON_FA_CHECK " OK##loadpalette")) {
			sceneMgr().loadPalette(_currentSelectedPalette, _searchFittingColors);
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

void PalettePanel::paletteActions(voxel::Palette &palette, command::CommandExecutionListener &listener) {
	ImGui::SliderFloat(ICON_FA_SLIDERS, &_intensityChange, -1.0f, 1.0f);
	ImGui::SameLine();
	const core::String &paletteChangeCmd = core::string::format("palette_changeintensity %f", _intensityChange);
	if (ImGui::CommandButton("Apply", paletteChangeCmd.c_str(), nullptr, ImVec2(0.0f, 0.0f), &listener)) {
		_intensityChange = 0.0f;
	}

	ImGui::CommandButton(ICON_FA_PALETTE " Load##palette", "importpalette", nullptr, ImVec2(0.0f, 0.0f), &listener);
	ImGui::SameLine();
	if (ImGui::Button("Switch##palette")) {
		reloadAvailablePalettes();
		ImGui::OpenPopup(POPUP_TITLE_LOAD_PALETTE);
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_FLOPPY_DISK " Export##savepalette")) {
		imguiApp()->saveDialog([&](const core::String &file, const io::FormatDescription *desc) { palette.save(file.c_str()); }, {}, io::format::palettes(), "palette.png");
	}
	ImGui::TooltipText("Export the palette");
}

void PalettePanel::closestColor(voxel::Palette &palette) {
	if (ImGui::ColorEdit4("Color closest match", glm::value_ptr(_closestColor), ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha)) {
		const core::RGBA rgba = core::Color::getRGBA(_closestColor);
		_closestMatch = palette.getClosestMatch(rgba);
	}
	ImGui::TooltipText("Select a color to find the closest match in the current loaded palette");
	ImGui::SameLine();
	char buf[256];
	core::string::formatBuf(buf, sizeof(buf), "%i##closestmatchpalpanel", _closestMatch);
	if (ImGui::Selectable(buf) && _closestMatch != -1) {
		voxel::VoxelType type;
		if (palette.colors[_closestMatch].a < 255) {
			type = voxel::VoxelType::Transparent;
		} else {
			type = voxel::VoxelType::Generic;
		}
		sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(type, _closestMatch));
	}
}

void PalettePanel::update(const char *title, command::CommandExecutionListener &listener) {
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
	const ImVec2 windowSize(10.0f * ImGui::GetFrameHeight(), ImGui::GetContentRegionMax().y);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const int currentSceneHoveredPalIdx = currentSceneColor();
	const int currentSelectedPalIdx = currentPaletteIndex();
	_hasFocus = false;
	_importPalette.clear();
	if (ImGui::Begin(title)) {
		_hasFocus = ImGui::IsWindowHovered();
		_colorHovered = false;

		const ImVec2 &pos = ImGui::GetCursorScreenPos();
		for (int palIdx = 0; palIdx < voxel::PaletteMaxColors; ++palIdx) {
			addColor(pos.x, palIdx, node, listener);
		}
		ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
		ImGui::Text("palette index: %i (scene voxel index %i)", currentSelectedPalIdx, currentSceneHoveredPalIdx);

		paletteActions(node.palette(), listener);
		closestColor(node.palette());
		createPopups();
	}

	if (core::Var::getSafe(cfg::VoxEditShowColorPicker)->boolVal()) {
		showColorPicker(currentSelectedPalIdx, node, listener);
	}

	ImGui::End();

	if (!_importPalette.empty()) {
		sceneMgr().importPalette(_importPalette);
	}
}

void PalettePanel::showColorPicker(uint8_t palIdx, voxelformat::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = node.palette();
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB;
	flags |= ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel;
	if (core::Var::getSafe(cfg::VoxEditColorWheel)->boolVal()) {
		flags |= ImGuiColorEditFlags_PickerHueWheel;
	} else {
		flags |= ImGuiColorEditFlags_PickerHueBar;
	}
	glm::vec4 color = core::Color::fromRGBA(palette.colors[palIdx]);
	const int maxPaletteEntries = palette.colorCount;
	const bool existingColor = palIdx < maxPaletteEntries;

	if (ImGui::ColorPicker4("Color", glm::value_ptr(color), flags)) {
		palette.colors[palIdx] = core::Color::getRGBA(color);
		palette.colors[palIdx].a = 255;
		if (!existingColor) {
			palette.colorCount = palIdx + 1;
		}
		palette.markDirty();
		palette.markSave();
		sceneMgr().mementoHandler().markPaletteChange(node);
	}
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

}
