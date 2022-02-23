/**
 * @file
 */

#include "PalettePanel.h"
#include "core/StringUtil.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"
#include "voxel/MaterialColor.h"

#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define PALETTEACTIONPOPUP "##paletteactionpopup"

namespace voxedit {

PalettePanel::PalettePanel() {
	_currentSelectedPalette = voxel::getDefaultPaletteName();
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
		const core::String& name = voxel::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
}

void PalettePanel::update(const char *title, command::CommandExecutionListener &listener) {
	const voxel::MaterialColorArray &colors = voxel::getMaterialColors();
	const int maxPaletteEntries = (int)colors.size();
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

			drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(colors[palIdx]));

			const core::String &id = core::string::format("##palitem-%i", palIdx);
			if (ImGui::InvisibleButton(id.c_str(), colorButtonSize)) {
				sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
			}
			const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
			if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
				const core::String &layerFromColorCmd = core::string::format("colortolayer %i", palIdx);
				ImGui::CommandMenuItem(ICON_FA_OBJECT_UNGROUP " Layer from color" PALETTEACTIONPOPUP, layerFromColorCmd.c_str(), true, &listener);
				if (voxel::materialColorIsGlow(palIdx)) {
					if (ImGui::MenuItem("Remove Glow")) {
						voxel::materialColorRemoveGlow(palIdx);
					}
				} else {
					if (ImGui::MenuItem("Glow")) {
						voxel::materialColorSetGlow(palIdx);
					}
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

		if (ImGui::BeginPopupModal(POPUP_TITLE_LOAD_PALETTE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Select the palette");
			ImGui::Separator();
			if (ImGui::BeginCombo(ICON_FA_TREE " Type", _currentSelectedPalette.c_str(), 0)) {
				for (const core::String& palette : _availablePalettes) {
					if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
						_currentSelectedPalette = palette;
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button(ICON_FA_CHECK " OK##loadpalette")) {
				sceneMgr().loadPalette(_currentSelectedPalette);
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
	}
	ImGui::End();
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

}
