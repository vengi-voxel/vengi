/**
 * @file
 */

#include "PalettePanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"

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
	const float height = ImGui::GetContentRegionMax().y;
	const float width = ImGui::Size(120.0f);
	const ImVec2 size(width, height);
	ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
	int voxelColorTraceIndex = sceneMgr().hitCursorVoxel().getColor();
	int voxelColorSelectedIndex = sceneMgr().modifier().cursorVoxel().getColor();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImVec2 pos = ImGui::GetWindowPos();
		pos.x += ImGui::GetWindowContentRegionMin().x;
		pos.y += ImGui::GetWindowContentRegionMin().y;
		const float size = ImGui::Size(20);
		const ImVec2& maxs = ImGui::GetWindowContentRegionMax();
		const ImVec2& mins = ImGui::GetWindowContentRegionMin();
		const int amountX = (int)((maxs.x - mins.x) / size);
		const int amountY = (int)((maxs.y - mins.y) / size);
		const int max = colors.size();
		int i = 0;
		float usedHeight = 0;
		bool colorHovered = false;
		for (int y = 0; y < amountY; ++y) {
			for (int x = 0; x < amountX; ++x) {
				if (i >= max) {
					break;
				}
				const float transX = pos.x + (float)x * size;
				const float transY = pos.y + (float)y * size;
				const ImVec2 v1(transX, transY);
				const ImVec2 v2(transX + (float)size, transY + (float)size);
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(colors[i]));

				if (!colorHovered && ImGui::IsMouseHoveringRect(v1, v2)) {
					colorHovered = true;
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Red));
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, i));
					}
				} else if (i == voxelColorTraceIndex) {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Yellow));
				} else if (i == voxelColorSelectedIndex) {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::DarkRed));
				} else {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Black));
				}
				++i;
			}
			if (i >= max) {
				break;
			}
			usedHeight += size;
		}

		ImGui::SetCursorPosY(pos.y + usedHeight);
		ImGui::Text("Color: %i (voxel %i)", voxelColorSelectedIndex, voxelColorTraceIndex);
		ImGui::TooltipText("Palette color index for current voxel under cursor");
		ImGui::CommandButton("Import palette", "importpalette", nullptr, 0.0f, &listener);
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
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

}
