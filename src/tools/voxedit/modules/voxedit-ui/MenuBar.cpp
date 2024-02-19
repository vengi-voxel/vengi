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
	if (ImGui::BeginCombo("Color reduction", colorReduction->strVal().c_str(), ImGuiComboFlags_None)) {
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
	ImGui::TooltipText(
		"The color reduction algorithm that is used when importing RGBA colors from images or rgba formats");
}

bool MenuBar::update(ui::IMGUIApp *app, command::CommandExecutionListener &listener) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginIconMenu(ICON_LC_FILE, "File")) {
			ImGui::CommandIconMenuItem(ICON_LC_SQUARE, "New", "new", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE_INPUT, "Load", "load", true, &listener);
			if (ImGui::BeginIconMenu(ICON_LC_FILE_STACK, "Recently opened")) {
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

			ImGui::CommandIconMenuItem(ICON_LC_SAVE, "Save", "save", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_SAVE, "Save as", "saveas", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_FILE, "Save selection", "exportselection", !sceneMgr().modifier().selections().empty(), &listener);
			ImGui::Separator();

			ImGui::CommandIconMenuItem(ICON_LC_PLUS_SQUARE, "Add file to scene", "import", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_PLUS_SQUARE, "Add directory to scene", "importdirectory", true, &listener);
			ImGui::Separator();
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, "Heightmap", "importheightmap", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, "Colored heightmap", "importcoloredheightmap", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, "Image as plane", "importplane", true, &listener);
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, "Image as volume", "importvolume", true, &listener);
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_DOOR_CLOSED, "Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_MENU, "Edit")) {
			const SceneManager &sceneManager = sceneMgr();
			const MementoHandler &mementoHandler = sceneManager.mementoHandler();
			ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CCW, "Undo", "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CW, "Redo", "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			const Modifier &modifier = sceneManager.modifier();
			const Selections &selections = modifier.selections();
			ImGui::CommandIconMenuItem(ICON_LC_SCISSORS, "Cut", "cut", !selections.empty(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_COPY, "Copy", "copy", !selections.empty(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, "Paste at reference##pastereferencepos", "paste",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, "Paste at cursor##pastecursor", "pastecursor",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandIconMenuItem(ICON_LC_CLIPBOARD_PASTE, "Paste as new node##pastenewnode", "pastenewnode",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::Separator();
			if (ImGui::BeginIconMenu(ICON_LC_MENU, "Options")) {
				ImGui::IconCheckboxVar(ICON_LC_GRID_3X3, "Grid", cfg::VoxEditShowgrid);
				ImGui::CheckboxVar("Show gizmo", cfg::VoxEditShowaxis);
				ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
				ImGui::IconCheckboxVar(ICON_LC_BOX, "Bounding box", cfg::VoxEditShowaabb);
				ImGui::IconCheckboxVar(ICON_LC_BONE, "Bones", cfg::VoxEditShowBones);
				ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() != (int)voxel::SurfaceExtractionType::Cubic);
				ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
				ImGui::EndDisabled();
				ImGui::CheckboxVar("Shadow", cfg::VoxEditRendershadow);
				ImGui::CheckboxVar("Bloom", cfg::ClientBloom);
				ImGui::CheckboxVar("Allow multi monitor", cfg::UIMultiMonitor);
				ImGui::CheckboxVar("Color picker", cfg::VoxEditShowColorPicker);
				ImGui::CheckboxVar("Color wheel", cfg::VoxEditColorWheel);
				ImGui::CheckboxVar("Simplified UI", cfg::VoxEditSimplifiedView);
				ImGui::CheckboxVar("Tip of the day", cfg::VoxEditTipOftheDay);

				ui::metricOption();

				static const core::Array<core::String, (int)voxel::SurfaceExtractionType::Max> meshModes = {
					"Cubes", "Marching cubes"};
				ImGui::ComboVar("Mesh mode", cfg::VoxelMeshMode, meshModes);
				ImGui::InputVarInt("Model animation speed", cfg::VoxEditAnimationSpeed);
				ImGui::InputVarInt("Autosave delay in seconds", cfg::VoxEditAutoSaveSeconds);
				ImGui::InputVarInt("Viewports", cfg::VoxEditViewports, 1, 1);
				ImGui::SliderVarFloat("Zoom speed", cfg::ClientCameraZoomSpeed, 0.1f, 200.0f);
				ImGui::SliderVarInt("View distance", cfg::VoxEditViewdistance, 10, 5000);
				ImGui::InputVarInt("Font size", cfg::UIFontSize, 1, 5);

				static const core::Array<core::String, ImGui::MaxStyles> uiStyles = {"CorporateGrey", "Dark", "Light",
																					 "Classic"};
				ImGui::ComboVar("Color theme", cfg::UIStyle, uiStyles);
				colorReductionOptions();

				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::ButtonFullWidth("Scene settings")) {
				core::Var::getSafe(cfg::VoxEditPopupSceneSettings)->setVal(true);
			}
			if (ImGui::ButtonFullWidth("Bindings")) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_SQUARE, "Select")) {
			ImGui::CommandMenuItem("None", "select none", true, &listener);
			ImGui::CommandMenuItem("Invert", "select invert", true, &listener);
			ImGui::CommandMenuItem("All", "select all", true, &listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_HELP_CIRCLE, "Help")) {
#ifdef DEBUG
			if (ImGui::BeginIconMenu(ICON_LC_BUG, "Debug")) {
				if (ImGui::Button("Textures")) {
					app->showTexturesDialog();
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::MenuItem("Tip of the day")) {
				core::Var::getSafe(cfg::VoxEditPopupTipOfTheDay)->setVal(true);
			}
			if (ImGui::MenuItem("Welcome screen")) {
				core::Var::getSafe(cfg::VoxEditPopupWelcome)->setVal(true);
			}
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_INFO, "About")) {
				core::Var::getSafe(cfg::VoxEditPopupAbout)->setVal(true);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxedit
