/**
 * @file
 */

#include "MenuBar.h"
#include "ViewMode.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/ConfigVar.h"
#include "imgui.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IMGUIStyle.h"
#include "ui/IconsLucide.h"
#include "ui/PopupAbout.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelui/FileDialogOptions.h"

namespace voxedit {

void MenuBar::viewportOptions() {
	ImGui::IconCheckboxVar(ICON_LC_GRID_3X3, _("Grid"), cfg::VoxEditShowgrid);
	ImGui::IconCheckboxVar(ICON_LC_ROTATE_3D, _("Show gizmo"), cfg::VoxEditShowaxis);
	ImGui::IconCheckboxVar(ICON_LC_LOCK, _("Show locked axis"), cfg::VoxEditShowlockedaxis);
	ImGui::IconCheckboxVar(ICON_LC_BOX, _("Bounding box"), cfg::VoxEditShowaabb);
	ImGui::IconCheckboxVar(ICON_LC_BONE, _("Bones"), cfg::VoxEditShowBones);
	ImGui::IconCheckboxVar(ICON_LC_FRAME, _("Plane"), cfg::VoxEditShowPlane);
	ImGui::IconSliderVarInt(ICON_LC_GRIP, _("Plane size"), cfg::VoxEditPlaneSize, 0, 1000);

	ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() == (int)voxel::SurfaceExtractionType::MarchingCubes);
	ImGui::IconCheckboxVar(ICON_LC_BOX, _("Outlines"), cfg::RenderOutline);
	if (viewModeNormalPalette(core::Var::getSafe(cfg::VoxEditViewMode)->intVal())) {
		ImGui::IconCheckboxVar(ICON_LC_BOX, _("Normals"), cfg::RenderNormals);
	}
	ImGui::IconCheckboxVar(ICON_LC_BRICK_WALL, _("Checkerboard"), cfg::RenderCheckerBoard);
	ImGui::EndDisabled();
	const char* shadingModeLabels[] = { _("Unlit"), _("Lit"), _("Shadows") };
	int currentShadingMode = core::Var::get(cfg::VoxEditShadingMode)->intVal();
	const char* currentLabel = (currentShadingMode >= 0 && currentShadingMode < (int)lengthof(shadingModeLabels)) ?
		shadingModeLabels[currentShadingMode] : _("Unknown");

	if (ImGui::BeginIconCombo(ICON_LC_SPOTLIGHT, _("Shading"), currentLabel)) {
		for (int i = 0; i < (int)lengthof(shadingModeLabels); ++i) {
			const bool isSelected = (currentShadingMode == i);
			if (ImGui::Selectable(shadingModeLabels[i], isSelected)) {
				core::Var::get(cfg::VoxEditShadingMode)->setVal(i);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::IconCheckboxVar(ICON_LC_SUN, _("Bloom"), cfg::ClientBloom);
	ImGui::IconSliderVarInt(ICON_LC_ECLIPSE, _("Tone mapping"), cfg::ToneMapping, 0, 3);
}

void MenuBar::init() {
}

void MenuBar::viewModeOption() {
	const core::Array<core::String, (int)ViewMode::AceOfSpades + 1> viewModes = {
		getViewModeString(ViewMode::Default),			// Default
		getViewModeString(ViewMode::Simple),			// Simple
		getViewModeString(ViewMode::All),				// All
		getViewModeString(ViewMode::TiberianSun),		// TiberianSun
		getViewModeString(ViewMode::RedAlert2),		// RedAlert2
		getViewModeString(ViewMode::MinecraftSkin),	// MinecraftSkin
		getViewModeString(ViewMode::AceOfSpades)		// AceOfSpades
	};
	static_assert(7 == (size_t)ViewMode::Max, "Unexpected viewmode array size");
	const core::VarPtr &viewMode = core::Var::getSafe(cfg::VoxEditViewMode);
	ImGui::ComboVar(_("View mode"), cfg::VoxEditViewMode, viewModes);
}

bool MenuBar::update(ui::IMGUIApp *app, command::CommandExecutionListener &listener) {
	core_trace_scoped(MenuBar);
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		ImGui::Dummy({});

		const scenegraph::SceneGraphNode *activeModelNode = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
		const bool hasSelection = activeModelNode && activeModelNode->hasSelection();

		if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE, _("New"), "new", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE_INPUT, _("Load"), "load", true, &listener);
			_app->lastOpenedMenu();

			ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save"), "save", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save as"), "saveas", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE, _("Save selection"), "exportselection", hasSelection, &listener);
			ImGui::Separator();

			ImGui::CommandIconMenuItem(ICON_LC_SQUARE_PLUS, _("Add file to scene"), "import", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE_PLUS, _("Add directory to scene"), "importdirectory", true,
									   &listener);
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_DOOR_CLOSED, _("Quit"))) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Edit"))) {
			const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
			ImGui::CommandIconMenuItem(ICON_LC_UNDO, _("Undo"), "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_REDO, _("Redo"), "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_COPY, _("Copy"), "copy", hasSelection, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SCISSORS, _("Cut"), "cut", hasSelection, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste at reference"), "paste",
									   _sceneMgr->clipboardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste at cursor"), "pastecursor",
									   _sceneMgr->clipboardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste as new node"), "pastenewnode",
									   _sceneMgr->clipboardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste as stamp"), "stampbrushpaste",
									   _sceneMgr->clipboardData(), &listener);
			ImGui::Separator();
			if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
				viewportOptions();
				ImGui::IconCheckboxVar(ICON_LC_TV_MINIMAL, _("Allow multi monitor"), cfg::UIMultiMonitor);
				ImGui::CheckboxVar(_("Color picker"), cfg::VoxEditShowColorPicker);
				ImGui::CheckboxVar(_("Color wheel"), cfg::VoxEditColorWheel);
				ImGui::IconCheckboxVar(ICON_LC_LIGHTBULB, _("Tip of the day"), cfg::VoxEditTipOftheDay);

				ui::metricOption();
				viewModeOption();
				_app->languageOption();

				voxelui::meshModeOption();
				ImGui::InputVarInt(_("Model animation speed"), cfg::VoxEditAnimationSpeed);
				ImGui::InputVarInt(_("Autosave delay in seconds"), cfg::VoxEditAutoSaveSeconds);
				ImGui::InputVarInt(_("Viewports"), cfg::VoxEditViewports, 1, 1);
				ImGui::SliderVarFloat(_("Zoom speed"), cfg::ClientCameraZoomSpeed, 0.1f, 200.0f);
				ImGui::SliderVarInt(_("View distance"), cfg::VoxEditViewdistance, 10, 5000);
				ImGui::InputVarInt(_("Font size"), cfg::UIFontSize, 1, 5);

				const core::VarPtr &uiStyleVar = core::Var::getSafe(cfg::UIStyle);
				int currentStyle = uiStyleVar->intVal();
				if (ImGui::BeginCombo(_("Color theme"), ImGui::GetStyleName(currentStyle))) {
					for (int i = 0; i < ImGui::MaxStyles; ++i) {
						const bool isSelected = (currentStyle == i);
						if (ImGui::Selectable(ImGui::GetStyleName(i), isSelected)) {
							uiStyleVar->setVal(i);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				_app->colorReductionOptions();

				ImGui::InputVarFloat(_("Notifications"), cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth(_("Reset layout"))) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::ButtonFullWidth(_("Bindings"))) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_SQUARE, _("Select"))) {
			ImGui::CommandIconMenuItem(ICON_LC_PIPETTE, _("Color picker"), "actioncolorpicker", true, &listener);
			ImGui::Separator();
			ImGui::CommandIconMenuItem(ICON_LC_SCAN, _("None"), "select none", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE_DASHED_MOUSE_POINTER, _("Select"), "brushselect", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SCAN, _("Invert"), "select invert", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SCAN, _("All"), "select all", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_CIRCLE_QUESTION_MARK, _("Help"))) {
#ifdef DEBUG
			if (ImGui::BeginIconMenu(ICON_LC_BUG, _("Debug"))) {
				if (ImGui::Button(_("Textures"))) {
					app->showTexturesDialog();
				}
				if (ImGui::Button(_("UI"))) {
					core::Var::getSafe(cfg::UIShowMetrics)->setVal(true);
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::IconMenuItem(ICON_LC_TERMINAL, _("Show all commands"))) {
				app->showCommandDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_SETTINGS, _("Show all cvars"))) {
				app->showCvarDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_TIMER, _("Show FPS stats"))) {
				app->showFPSDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_LIGHTBULB, _("Tip of the day"))) {
				core::Var::getSafe(cfg::VoxEditPopupTipOfTheDay)->setVal(true);
			}
			if (ImGui::MenuItem(_("Welcome screen"))) {
				core::Var::getSafe(cfg::VoxEditPopupWelcome)->setVal(true);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_("Minecraft mapping"))) {
				core::Var::getSafe(cfg::VoxEditPopupMinecraftMapping)->setVal(true);
			}
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
				core::Var::getSafe(cfg::VoxEditPopupAbout)->setVal(true);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxedit
