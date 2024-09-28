/**
 * @file
 */

#include "NormalPalettePanel.h"
#include "IMGUIEx.h"
#include "IconsLucide.h"
#include "app/I18N.h"
#include "imgui.h"
#include "ui/IMGUIApp.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NormalPalettePanel::NormalPalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
	: Super(app, "normalpalette"), _sceneMgr(sceneMgr) {
}

void NormalPalettePanel::addColor(float startingPosX, uint8_t paletteColorIdx, scenegraph::SceneGraphNode &node,
								  command::CommandExecutionListener &listener) {
	palette::NormalPalette &normalPalette = node.normalPalette();
	const int maxPaletteEntries = normalPalette.size();
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
	const bool usableColor = normalPalette.normal(paletteColorIdx).a > 0;
	const bool existingColor = paletteColorIdx < maxPaletteEntries;
	if (existingColor) {
		const core::RGBA color = normalPalette.normal(paletteColorIdx);
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
	ImGui::InvisibleButton("", colorButtonSize);
	ImGui::PopID();
	globalCursorPos.x += colorButtonSize.x;
	if (globalCursorPos.x > windowPos.x + contentRegionWidth - colorButtonSize.x) {
		globalCursorPos.x = startingPosX;
		globalCursorPos.y += colorButtonSize.y;
	}
	ImGui::SetCursorScreenPos(globalCursorPos);
	// restore the draw list flags from above
	drawList->Flags = backupFlags;
}

void NormalPalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	palette::NormalPalette &normalPalette = node.normalPalette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_PALETTE, _("File"))) {
			if (ImGui::MenuItem(_("Tiberan Sun"))) {
				normalPalette.tiberianSun();
			}
			if (ImGui::MenuItem(_("Red Alert 2"))) {
				normalPalette.redAlert2();
			}
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Export"))) {
				_app->saveDialog([&](const core::String &file,
									 const io::FormatDescription *desc) { normalPalette.save(file.c_str()); },
								 {}, io::format::palettes(), "palette.png");
			}
			ImGui::TooltipTextUnformatted(_("Export the palette"));
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void NormalPalettePanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(NormalPalettePanel);
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int nodeId = sceneGraph.activeNode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionHeight = available.y + ImGui::GetCursorPosY();
	const ImVec2 windowSize(10.0f * ImGui::GetFrameHeight(), contentRegionHeight);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
	const core::String title = makeTitle(ICON_LC_PALETTE, _("Normals"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
		if (node.isModelNode()) {
			paletteMenuBar(node, listener);
			const ImVec2 &pos = ImGui::GetCursorScreenPos();
			const palette::Palette &palette = node.palette();
			for (int palettePanelIdx = 0; palettePanelIdx < palette::PaletteMaxColors; ++palettePanelIdx) {
				const uint8_t paletteColorIdx = palette.uiIndex(palettePanelIdx);
				addColor(pos.x, paletteColorIdx, node, listener);
			}
		}
	}

	ImGui::End();
}

} // namespace voxedit