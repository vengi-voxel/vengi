/**
 * @file
 */

#include "MenuBar.h"
#include "OptionsPanel.h"
#include "ViewMode.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/ConfigVar.h"
#include "core/collection/Array.h"
#include "imgui.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IMGUIStyle.h"
#include "ui/IconsLucide.h"
#include "ui/PopupAbout.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/SurfaceExtractor.h"

namespace voxedit {

void MenuBar::viewportOptions() {
	ImGui::IconCheckboxVar(ICON_LC_GRID_3X3, cfg::VoxEditShowgrid);
	ImGui::IconCheckboxVar(ICON_LC_ROTATE_3D, cfg::VoxEditShowaxis);
	ImGui::IconCheckboxVar(ICON_LC_LOCK, cfg::VoxEditShowlockedaxis);
	ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::VoxEditShowaabb);
	ImGui::IconCheckboxVar(ICON_LC_BONE, cfg::VoxEditShowBones);
	ImGui::IconCheckboxVar(ICON_LC_FRAME, cfg::VoxEditShowPlane);
	ImGui::IconSliderVarInt(ICON_LC_GRIP, cfg::VoxEditPlaneSize);

	ImGui::BeginDisabled(core::getVar(cfg::VoxRenderMeshMode)->intVal() == (int)voxel::SurfaceExtractionType::MarchingCubes);
	ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::RenderOutline);
	if (viewModeNormalPalette(core::getVar(cfg::VoxEditViewMode)->intVal())) {
		ImGui::IconCheckboxVar(ICON_LC_BOX, cfg::RenderNormals);
	}
	ImGui::IconCheckboxVar(ICON_LC_BRICK_WALL, cfg::RenderCheckerBoard);
	ImGui::EndDisabled();
	const char* shadingModeLabels[] = { _("Unlit"), _("Lit"), _("Shadows") };
	int currentShadingMode = core::getVar(cfg::VoxEditShadingMode)->intVal();
	const char* currentLabel = (currentShadingMode >= 0 && currentShadingMode < (int)lengthof(shadingModeLabels)) ?
		shadingModeLabels[currentShadingMode] : _("Unknown");

	if (ImGui::BeginIconCombo(ICON_LC_SPOTLIGHT, _("Shading"), currentLabel)) {
		for (int i = 0; i < (int)lengthof(shadingModeLabels); ++i) {
			const bool isSelected = (currentShadingMode == i);
			if (ImGui::Selectable(shadingModeLabels[i], isSelected)) {
				core::getVar(cfg::VoxEditShadingMode)->setVal(i);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::IconCheckboxVar(ICON_LC_SUN, cfg::ClientBloom);
	ImGui::IconSliderVarInt(ICON_LC_ECLIPSE, cfg::ToneMapping);
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
	const core::VarPtr &viewMode = core::getVar(cfg::VoxEditViewMode);
	ImGui::ComboVar(viewMode, viewModes);
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
			if (ImGui::BeginIconMenu(ICON_LC_CIRCLE, _("Record"))) {
				const bool isRecording = _sceneMgr->isRecording();
				const bool isPlaying = _sceneMgr->isPlaying();
				if (!isRecording) {
					ImGui::CommandIconMenuItem(ICON_LC_CIRCLE, _("Start recording"), "record_start", !isPlaying, &listener);
				} else {
					ImGui::CommandIconMenuItem(ICON_LC_SQUARE, _("Stop recording"), "record_stop", true, &listener);
				}
				ImGui::Separator();
				if (!isPlaying) {
					ImGui::CommandIconMenuItem(ICON_LC_PLAY, _("Playback"), "record_playback", !isRecording, &listener);
				} else {
					ImGui::CommandIconMenuItem(ICON_LC_SQUARE, _("Stop playback"), "record_playback_stop", true, &listener);
					const bool isPaused = _sceneMgr->isPlaybackPaused();
					if (isPaused) {
						ImGui::CommandIconMenuItem(ICON_LC_PLAY, _("Resume"), "record_playback_resume", true, &listener);
					} else {
						ImGui::CommandIconMenuItem(ICON_LC_PAUSE, _("Pause"), "record_playback_pause", true, &listener);
					}
				}
				float speed = _sceneMgr->playbackSpeed();
				if (ImGui::SliderFloat(_("Speed (s)"), &speed, 0.0f, 2.0f, "%.1f s")) {
					_sceneMgr->setPlaybackSpeed(speed);
				}
				ImGui::EndMenu();
			}
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
			if (ImGui::IconMenuItem(ICON_LC_SETTINGS, _("Options"))) {
				if (_optionsPanel) {
					_optionsPanel->toggleVisible();
				}
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
					core::getVar(cfg::UIShowMetrics)->setVal(true);
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::IconMenuItem(ICON_LC_TERMINAL, _("Show all commands"))) {
				app->showCommandDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_TIMER, _("Show FPS stats"))) {
				app->showFPSDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_LIGHTBULB, _("Tip of the day"))) {
				core::getVar(cfg::VoxEditPopupTipOfTheDay)->setVal(true);
			}
			if (ImGui::MenuItem(_("Welcome screen"))) {
				core::getVar(cfg::VoxEditPopupWelcome)->setVal(true);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_("Minecraft mapping"))) {
				core::getVar(cfg::VoxEditPopupMinecraftMapping)->setVal(true);
			}
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
				core::getVar(cfg::VoxEditPopupAbout)->setVal(true);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	if (_optionsPanel && _optionsPanel->shouldResetDockLayout()) {
		resetDockLayout = true;
	}
	return resetDockLayout;
}

} // namespace voxedit
