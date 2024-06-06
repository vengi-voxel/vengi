/**
 * @file
 */

#include "MenuBar.h"
#include "IMGUIApp.h"
#include "IMGUIStyle.h"
#include "PopupAbout.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/StringUtil.h"
#include "engine-config.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/SurfaceExtractor.h"

namespace voxedit {

void MenuBar::colorReductionOptions() {
	const core::VarPtr &colorReduction = core::Var::getSafe(cfg::CoreColorReduction);
	if (ImGui::BeginCombo(_("Color reduction"), colorReduction->strVal().c_str(), ImGuiComboFlags_None)) {
		core::Color::ColorReductionType type = core::Color::toColorReductionType(colorReduction->strVal().c_str());
		for (int i = 0; i < (int)core::Color::ColorReductionType::Max; ++i) {
			const bool selected = i == (int)type;
			const char *str = core::Color::toColorReductionTypeString((core::Color::ColorReductionType)i);
			if (ImGui::Selectable(str, selected)) {
				colorReduction->setVal(str);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::TooltipTextUnformatted(
		_("The color reduction algorithm that is used when importing RGBA colors from images or rgba formats"));
}

void MenuBar::viewportOptions() {
	ImGui::IconCheckboxVar(ICON_LC_GRID_3X3, _("Grid"), cfg::VoxEditShowgrid);
	ImGui::IconCheckboxVar(ICON_LC_ROTATE_3D, _("Show gizmo"), cfg::VoxEditShowaxis);
	ImGui::IconCheckboxVar(ICON_LC_LOCK, _("Show locked axis"), cfg::VoxEditShowlockedaxis);
	ImGui::IconCheckboxVar(ICON_LC_BOX, _("Bounding box"), cfg::VoxEditShowaabb);
	ImGui::IconCheckboxVar(ICON_LC_BONE, _("Bones"), cfg::VoxEditShowBones);
	ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() != (int)voxel::SurfaceExtractionType::Cubic);
	ImGui::IconCheckboxVar(ICON_LC_BOX, _("Outlines"), cfg::RenderOutline);
	ImGui::EndDisabled();
	ImGui::IconCheckboxVar(ICON_LC_SUNSET, _("Shadow"), cfg::VoxEditRendershadow);
	ImGui::IconCheckboxVar(ICON_LC_SUN, _("Bloom"), cfg::ClientBloom);
	ImGui::IconSliderVarInt(ICON_LC_ECLIPSE, _("Tone mapping"), cfg::ToneMapping, 0, 3);
}

bool MenuBar::update(ui::IMGUIApp *app, command::CommandExecutionListener &listener) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE, _("New"), "new", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE_INPUT, _("Load"), "load", true, &listener);
			if (ImGui::BeginIconMenu(ICON_LC_FILE_STACK, _("Recently opened"))) {
				int recentlyOpened = 0;
				for (const core::String &f : _lastOpenedFiles) {
					if (f.empty()) {
						break;
					}
					const core::String &item = core::string::format("%s##%i", f.c_str(), recentlyOpened);
					if (ImGui::MenuItem(item.c_str())) {
						command::executeCommands("load \"" + f + "\"", &listener);
					}
					++recentlyOpened;
				}
				ImGui::EndMenu();
			}

			ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save"), "save", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SAVE, _("Save as"), "saveas", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE, _("Save selection"), "exportselection", !_sceneMgr->modifier().selections().empty(), &listener);
			ImGui::Separator();

			ImGui::CommandIconMenuItem(ICON_LC_SQUARE_PLUS, _("Add file to scene"), "import", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE_PLUS, _("Add directory to scene"), "importdirectory", true, &listener);
			ImGui::Separator();
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, _("Heightmap"), "importheightmap", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, _("Colored heightmap"), "importcoloredheightmap", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, _("Image as plane"), "importplane", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, _("Image as volume"), "importvolume", true, &listener);
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_DOOR_CLOSED, _("Quit"))) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Edit"))) {
			const MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
			ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CCW, _("Undo"), "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CW, _("Redo"), "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			const Modifier &modifier = _sceneMgr->modifier();
			const Selections &selections = modifier.selections();
			ImGui::CommandIconMenuItem(ICON_LC_SCISSORS, _("Cut"), "cut", !selections.empty(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_COPY, _("Copy"), "copy", !selections.empty(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste at reference"), "paste",
								   _sceneMgr->clipBoardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste at cursor"), "pastecursor",
								   _sceneMgr->clipBoardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste as new node"), "pastenewnode",
								   _sceneMgr->clipBoardData(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste as stamp"), "stampbrushpaste",
								   _sceneMgr->clipBoardData(), &listener);
			ImGui::Separator();
			if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
				viewportOptions();
				ImGui::IconCheckboxVar(ICON_LC_TV_2, _("Allow multi monitor"), cfg::UIMultiMonitor);
				ImGui::CheckboxVar(_("Color picker"), cfg::VoxEditShowColorPicker);
				ImGui::CheckboxVar(_("Color wheel"), cfg::VoxEditColorWheel);
				ImGui::CheckboxVar(_("Simplified UI"), cfg::VoxEditSimplifiedView);
				ImGui::CheckboxVar(_("Tip of the day"), cfg::VoxEditTipOftheDay);

				ui::metricOption();
				_app->languageOption();

				static const core::Array<core::String, (int)voxel::SurfaceExtractionType::Max> meshModes = {
					_("Cubes"), _("Marching cubes")};
				ImGui::ComboVar(_("Mesh mode"), cfg::VoxelMeshMode, meshModes);
				ImGui::InputVarInt(_("Model animation speed"), cfg::VoxEditAnimationSpeed);
				ImGui::InputVarInt(_("Autosave delay in seconds"), cfg::VoxEditAutoSaveSeconds);
				ImGui::InputVarInt(_("Viewports"), cfg::VoxEditViewports, 1, 1);
				ImGui::SliderVarFloat(_("Zoom speed"), cfg::ClientCameraZoomSpeed, 0.1f, 200.0f);
				ImGui::SliderVarInt(_("View distance"), cfg::VoxEditViewdistance, 10, 5000);
				ImGui::InputVarInt(_("Font size"), cfg::UIFontSize, 1, 5);

				static const core::Array<core::String, ImGui::MaxStyles> uiStyles = {"CorporateGrey", "Dark", "Light",
																					 "Classic"};
				ImGui::ComboVar(_("Color theme"), cfg::UIStyle, uiStyles);
				colorReductionOptions();

				ImGui::InputVarFloat(_("Notifications"), cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth(_("Reset layout"))) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::ButtonFullWidth(_("Scene settings"))) {
				core::Var::getSafe(cfg::VoxEditPopupSceneSettings)->setVal(true);
			}
			if (ImGui::ButtonFullWidth(_("Bindings"))) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_SQUARE, _("Select"))) {
			ImGui::CommandMenuItem(_("None"), "select none", true, &listener);
			ImGui::CommandMenuItem(_("Invert"), "select invert", true, &listener);
			ImGui::CommandMenuItem(_("All"), "select all", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_CIRCLE_HELP, _("Help"))) {
#ifdef DEBUG
			if (ImGui::BeginIconMenu(ICON_LC_BUG, _("Debug"))) {
				if (ImGui::Button(_("Textures"))) {
					app->showTexturesDialog();
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::MenuItem(_("Show all commands"))) {
				app->showCommandDialog();
			}
			if (ImGui::MenuItem(_("Show all cvars"))) {
				app->showCvarDialog();
			}
			if (ImGui::MenuItem(_("Tip of the day"))) {
				core::Var::getSafe(cfg::VoxEditPopupTipOfTheDay)->setVal(true);
			}
			if (ImGui::MenuItem(_("Welcome screen"))) {
				core::Var::getSafe(cfg::VoxEditPopupWelcome)->setVal(true);
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
