/**
 * @file
 */

#include "PalettePanel.h"
#include "IMGUIApp.h"
#include "core/StringUtil.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"
#include <glm/gtc/type_ptr.hpp>

#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define PALETTEACTIONPOPUP "##paletteactionpopup"

namespace voxedit {

PalettePanel::PalettePanel() {
	_currentSelectedPalette = voxel::Palette::getDefaultPaletteName();
}

void PalettePanel::reloadAvailablePalettes() {
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	io::filesystem()->list("", entities, "palette-*.png");
	if (entities.empty()) {
		Log::error("Could not find any palettes");
	}
	_availablePalettes.clear();
	for (const io::Filesystem::DirEntry& file : entities) {
		if (file.type != io::Filesystem::DirEntry::Type::file) {
			continue;
		}
		const core::String& name = voxel::Palette::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
}

void PalettePanel::update(const char *title, command::CommandExecutionListener &listener) {
	voxel::Palette &palette = voxel::getPalette();
	const int maxPaletteEntries = palette.colorCount;
	const float height = ImGui::GetContentRegionMax().y;
	const ImVec2 windowSize(120.0f, height);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const int currentSceneHoveredPalIdx = sceneMgr().hitCursorVoxel().getColor();
	const int currentSelectedPalIdx = sceneMgr().modifier().cursorVoxel().getColor();
	_hasFocus = false;
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		_hasFocus = ImGui::IsWindowHovered();
		const ImVec2 colorButtonSize(20, 20);
		const ImVec2 &pos = ImGui::GetCursorScreenPos();
		bool colorHovered = false;
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImDrawListFlags backupFlags = drawList->Flags;
		drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		const ImU32 redColor = ImGui::GetColorU32(core::Color::Red);
		const ImU32 yellowColor = ImGui::GetColorU32(core::Color::Yellow);
		const ImU32 darkRedColor = ImGui::GetColorU32(core::Color::DarkRed);

		const float windowWidth = ImGui::GetWindowContentRegionMax().x;
		ImVec2 globalCursorPos = ImGui::GetCursorScreenPos();
		for (int palIdx = 0; palIdx < maxPaletteEntries; ++palIdx) {
			const float borderWidth = 1.0f;
			const ImVec2 v1(globalCursorPos.x + borderWidth, globalCursorPos.y + borderWidth);
			const ImVec2 v2(globalCursorPos.x + colorButtonSize.x, globalCursorPos.y + colorButtonSize.y);

			drawList->AddRectFilled(v1, v2, palette.colors[palIdx]);

			static glm::vec4 color;
			color = core::Color::fromRGBA(palette.colors[palIdx]);
			const core::String &id = core::string::format("##palitem-%i", palIdx);
			if (ImGui::InvisibleButton(id.c_str(), colorButtonSize)) {
				voxel::VoxelType type;
				if (color.a < 1.0f) {
					type = voxel::VoxelType::Transparent;
				} else {
					type = voxel::VoxelType::Generic;
				}
				sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(type, palIdx));
			}
			const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
			if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
				const core::String &layerFromColorCmd = core::string::format("colortolayer %i", palIdx);
				ImGui::CommandMenuItem(ICON_FA_OBJECT_UNGROUP " Layer from color" PALETTEACTIONPOPUP, layerFromColorCmd.c_str(), true, &listener);
				if (palette.hasGlow(palIdx)) {
					if (ImGui::MenuItem("Remove Glow")) {
						palette.removeGlow(palIdx);
					}
				} else {
					if (ImGui::MenuItem("Glow")) {
						palette.setGlow(palIdx);
					}
				}
				if (ImGui::ColorEdit4("Color", glm::value_ptr(color), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float)) {
					palette.colors[palIdx] = core::Color::getRGBA(color);
					palette.markDirty();
					palette.markSave();
				}

				ImGui::EndPopup();
			}
			if (!colorHovered && ImGui::IsItemHovered()) {
				colorHovered = true;
				drawList->AddRect(v1, v2, redColor);
			} else if (palIdx == currentSceneHoveredPalIdx) {
				drawList->AddRect(v1, v2, yellowColor);
			} else if (palIdx == currentSelectedPalIdx) {
				drawList->AddRect(v1, v2, darkRedColor);
			}
			globalCursorPos.x += colorButtonSize.x;
			if (globalCursorPos.x > windowWidth - colorButtonSize.x) {
				globalCursorPos.x = pos.x;
				globalCursorPos.y += colorButtonSize.y;
			}
			ImGui::SetCursorScreenPos(globalCursorPos);
		}
		const ImVec2 cursorPos(pos.x, globalCursorPos.y + colorButtonSize.y);
		ImGui::SetCursorScreenPos(cursorPos);

		// restore the draw list flags from above
		drawList->Flags = backupFlags;

		ImGui::Text("Color: %i (voxel %i)", currentSelectedPalIdx, currentSceneHoveredPalIdx);
		ImGui::TooltipText("Palette color index for current voxel under cursor");
		ImGui::CommandButton(ICON_FA_PALETTE " Import palette", "importpalette", nullptr, 0.0f, &listener);
		ImGui::SameLine();
		if (ImGui::Button("Load palette##button")) {
			reloadAvailablePalettes();
			ImGui::OpenPopup(POPUP_TITLE_LOAD_PALETTE);
		}

		static const char *builtIn[] = {"built-in:minecraft", "built-in:magicavoxel"};
		if (ImGui::BeginPopupModal(POPUP_TITLE_LOAD_PALETTE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Select the palette");
			ImGui::Separator();
			if (ImGui::BeginCombo(ICON_FA_TREE " Type", _currentSelectedPalette.c_str(), 0)) {
				for (const core::String& palette : _availablePalettes) {
					if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
						_currentSelectedPalette = palette;
					}
				}
				for (int i = 0; i < lengthof(builtIn); ++i) {
					if (ImGui::Selectable(builtIn[i], builtIn[i] == _currentSelectedPalette)) {
						_currentSelectedPalette = builtIn[i];
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button(ICON_FA_CHECK " OK##loadpalette")) {
				if (_currentSelectedPalette == builtIn[0]) {
					palette.minecraft();
				} else if (_currentSelectedPalette == builtIn[1]) {
					palette.magicaVoxel();
				} else {
					sceneMgr().loadPalette(_currentSelectedPalette);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES " Cancel##loadpalette")) {
				ImGui::CloseCurrentPopup();
			}
			// TODO: link to docu on how to add new palettes
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}

		if (palette.needsSave()) {
			if (ImGui::Button(ICON_FA_SAVE " Save##savepalette")) {
				if (!palette.save()) {
					imguiApp()->saveDialog([&](const core::String &file) { palette.save(file.c_str()); });
				}
				palette.markSaved();
			}
			ImGui::TooltipText("Save the modified palette");
		}
	}
	ImGui::End();
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

}
