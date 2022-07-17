/**
 * @file
 */

#include "PalettePanel.h"
#include "IMGUIApp.h"
#include "DragAndDropPayload.h"
#include "core/StringUtil.h"
#include "imgui.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsForkAwesome.h"
#include <glm/gtc/type_ptr.hpp>

#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define PALETTEACTIONPOPUP "##paletteactionpopup"

namespace voxedit {

PalettePanel::PalettePanel() {
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
}

void PalettePanel::update(const char *title, command::CommandExecutionListener &listener) {
	core::String importPalette;
	voxel::Palette &palette = sceneMgr().activePalette();
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

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::Text("Color %i", palIdx);
				ImGui::SetDragDropPayload(dragdrop::ColorPayload, (const void*)&palIdx, sizeof(int), ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::ColorPayload)) {
					const int dragPalIdx = *(int*)payload->Data;
					core::exchange(palette.colors[palIdx], palette.colors[dragPalIdx]);
					palette.markDirty();
				}
				if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(dragdrop::ImagePayload)) {
					const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
					importPalette = image->name();
				}
				ImGui::EndDragDropTarget();
			}

			const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
			if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
				static bool pickerWheel = false;
				ImGui::Checkbox("Wheel", &pickerWheel);
				ImGuiColorEditFlags flags = ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB;
				flags |= ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoAlpha;
				if (pickerWheel)  {
					flags |= ImGuiColorEditFlags_PickerHueWheel;
				} else {
					flags |= ImGuiColorEditFlags_PickerHueBar;
				}
				if (ImGui::ColorPicker4("Color", glm::value_ptr(color), flags)) {
					palette.colors[palIdx] = core::Color::getRGBA(color);
					palette.markDirty();
					palette.markSave();
				}
				if (ImGui::CollapsingHeader("Commands", ImGuiTreeNodeFlags_DefaultOpen)) {
					const core::String &layerFromColorCmd = core::string::format("colortolayer %i", palIdx);
					ImGui::CommandMenuItem(ICON_FA_OBJECT_UNGROUP " Layer from color" PALETTEACTIONPOPUP, layerFromColorCmd.c_str(), true, &listener);
				}

				if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
					if (palette.hasGlow(palIdx)) {
						if (ImGui::MenuItem(ICON_FK_SUN_O " Remove Glow")) {
							palette.removeGlow(palIdx);
						}
					} else {
						if (ImGui::MenuItem(ICON_FK_SUN " Glow")) {
							palette.setGlow(palIdx);
						}
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

		static const char *builtIn[] = {"built-in:minecraft", "built-in:magicavoxel", "built-in:nippon"};
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
				} else if (_currentSelectedPalette == builtIn[2]) {
					palette.nippon();
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

		ImGui::Dummy(ImVec2(10, 10));

		static int closestMatch = -1;
		static glm::vec4 closestColor{0.0f};
		if (ImGui::ColorEdit4("Color closest match", glm::value_ptr(closestColor), ImGuiColorEditFlags_NoInputs)) {
			closestMatch = palette.getClosestMatch(closestColor);
		}
		ImGui::TooltipText("Select a color to find the closest match in the current loaded palette");
		ImGui::SameLine();
		char buf[256];
		core::string::formatBuf(buf, sizeof(buf), "%i##closestmatchpalpanel", closestMatch);
		if (ImGui::Selectable(buf) && closestMatch != -1) {
			glm::vec4 color = core::Color::fromRGBA(palette.colors[closestMatch]);
			voxel::VoxelType type;
			if (color.a < 1.0f) {
				type = voxel::VoxelType::Transparent;
			} else {
				type = voxel::VoxelType::Generic;
			}
			sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(type, closestMatch));
		}
	}
	ImGui::End();

	if (!importPalette.empty()) {
		sceneMgr().importPalette(importPalette);
	}
}

bool PalettePanel::hasFocus() const {
	return _hasFocus;
}

}
