/**
 * @file
 */

#include "NormalPalettePanel.h"
#include "app/I18N.h"
#include "color/Color.h"
#include "command/CommandHandler.h"
#include "core/Trace.h"
#include "imgui.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteFormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/dearimgui/imgui_internal.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NormalPalettePanel::NormalPalettePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
	: Super(app, "normalpalette"), _sceneMgr(sceneMgr) {
}

void NormalPalettePanel::init() {
	_renderNormals = core::Var::getSafe(cfg::RenderNormals);
}

void NormalPalettePanel::addColor(ImVec2 &cursorPos, float startingPosX, float contentRegionRightEdge,
								  uint8_t paletteColorIdx, float colorButtonSize, scenegraph::SceneGraphNode &node,
								  command::CommandExecutionListener &listener) {
	core_trace_scoped(AddColor);
	palette::NormalPalette &normalPalette = node.normalPalette();
	const int maxPaletteEntries = normalPalette.size();
	const float borderWidth = 1.0f;
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImVec2 v1(cursorPos.x + borderWidth, cursorPos.y + borderWidth);
	const ImVec2 v2(cursorPos.x + colorButtonSize, cursorPos.y + colorButtonSize);
	const bool existingColor = paletteColorIdx < maxPaletteEntries;
	color::RGBA color;
	if (existingColor) {
		color = normalPalette.normal(paletteColorIdx);
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
		color = color::RGBA(0, 0, 0, 0);
		drawList->AddRect(v1, v2, color::RGBA(0, 0, 0, 255));
	}

	const bool usableColor = color.a > 0;
	const ImGuiID id = ImGui::GetID((int)paletteColorIdx);
	const ImRect bb(cursorPos, ImVec2(cursorPos.x + colorButtonSize, cursorPos.y + colorButtonSize));

	bool hovered = false;
	bool held = false;
	const bool isMouseHovering = bb.Contains(ImGui::GetMousePos());
	const bool isActive = (id == ImGui::GetActiveID());

	if (isMouseHovering || isActive) {
		if (ImGui::ItemAdd(bb, id)) {
			if (usableColor && ImGui::ButtonBehavior(bb, id, &hovered, &held)) {
				_selectedIndex = paletteColorIdx;
				_sceneMgr->modifier().setNormalColorIndex(paletteColorIdx);
			}
		}
	}

	if (hovered) {
		drawList->AddRect(v1, v2, ImGui::GetColorU32(color::Red()), 0.0f, 0, 2.0f);
	} else if (_selectedIndex == paletteColorIdx) {
		drawList->AddRect(v1, v2, ImGui::GetColorU32(color::Yellow()), 0.0f, 0, 2.0f);
	}

	cursorPos.x += colorButtonSize;
	if (cursorPos.x > contentRegionRightEdge - colorButtonSize) {
		cursorPos.x = startingPosX;
		cursorPos.y += colorButtonSize;
	}
}

void NormalPalettePanel::paletteMenuBar(scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener) {
	palette::NormalPalette &normalPalette = node.normalPalette();
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_PALETTE, _("File"))) {
			if (ImGui::MenuItem(_("Tiberian Sun"))) {
				const core::String &cmd = core::String::format("normalpalette %s", palette::NormalPalette::builtIn[1]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::MenuItem(_("Red Alert 2"))) {
				const core::String &cmd = core::String::format("normalpalette %s", palette::NormalPalette::builtIn[0]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::MenuItem(_("Slab6"))) {
				const core::String &cmd = core::String::format("normalpalette %s", palette::NormalPalette::builtIn[2]);
				command::executeCommands(cmd, &listener);
			}
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Export"))) {
				_app->saveDialog([&](const core::String &file,
									 const io::FormatDescription *desc) { normalPalette.save(file.c_str()); },
								 {}, palette::palettes(), "palette.png");
			}
			if (ImGui::BeginMenu(_("Auto normals"))) {
				const char *normalModes[] = {_("Flat"), _("Smooth"), _("Smoother")};
				const core::VarPtr &normalModeVar = core::Var::getSafe(cfg::VoxEditAutoNormalMode);
				const int currentNormalMode = normalModeVar->intVal();

				if (ImGui::BeginCombo(_("Normal mode"), normalModes[currentNormalMode])) {
					for (int i = 0; i < lengthof(normalModes); ++i) {
						const char *normalMode = normalModes[i];
						if (normalMode == nullptr) {
							continue;
						}
						const bool selected = i == currentNormalMode;
						if (ImGui::Selectable(normalMode, selected)) {
							normalModeVar->setVal(core::string::toString(i));
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::Checkbox(_("Recalculate all normals"), &_recalcAll);
				ImGui::SetItemTooltipUnformatted(_("If the model already has normals and you want to replace them"));
				ImGui::Checkbox(_("Model is hollow"), &_onlySurfaceVoxels);
				ImGui::SetItemTooltipUnformatted(
					_("Fill hollows to re-calculate the normals and\nhollow the model afterwards again.\n\n"
					  "For calculating normals it is needed that the model has a closed\n"
					  "surface and the hollow area is filled.\n\n"
					  "Either do it manually or activate this option."));
				if (ImGui::IconMenuItem(ICON_LC_PLAY, _("Calculate normals"))) {
					voxel::Connectivity connectivity = voxel::Connectivity::SixConnected;
					if (currentNormalMode == 1) {
						connectivity = voxel::Connectivity::EighteenConnected;
					} else if (currentNormalMode == 2) {
						connectivity = voxel::Connectivity::TwentySixConnected;
					}
					_sceneMgr->calculateNormals(node.id(), connectivity, _recalcAll, _onlySurfaceVoxels);
				}

				ImGui::EndMenu();
			}
			ImGui::TooltipTextUnformatted(_("Calculate normals for the model"));
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
	_sceneMgr->modifier().setNormalPaint(_renderNormals->boolVal());
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
		if (node.isModelNode()) {
			paletteMenuBar(node, listener);
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			const float startingPosX = cursorPos.x;
			const float availableX = ImGui::GetContentRegionAvail().x;
			const float contentRegionRightEdge = availableX + cursorPos.x;
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			const ImDrawListFlags backupFlags = drawList->Flags;
			drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;
			const float frameHeight = ImGui::GetFrameHeight();

			for (int palettePanelIdx = 0; palettePanelIdx < palette::PaletteMaxColors; ++palettePanelIdx) {
				addColor(cursorPos, startingPosX, contentRegionRightEdge, (uint8_t)palettePanelIdx, frameHeight, node,
						 listener);
			}

			ImGui::SetCursorScreenPos(cursorPos);

			drawList->Flags = backupFlags;
			ImGui::Dummy(ImVec2(0, frameHeight));

			ImGui::CheckboxVar(_("Render normals"), _renderNormals);
		}
	}

	ImGui::End();
}

} // namespace voxedit
