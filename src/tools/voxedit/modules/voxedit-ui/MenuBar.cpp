/**
 * @file
 */

#include "MenuBar.h"
#include "IMGUIApp.h"
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

bool MenuBar::actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener) {
	return ImGui::CommandMenuItem(title, command, true, &listener);
}

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

void MenuBar::metricOption() {
	const core::VarPtr &metricFlavor = core::Var::getSafe(cfg::MetricFlavor);
	bool metrics = !metricFlavor->strVal().empty();
	if (ImGui::Checkbox("Enable sending anonymous metrics", &metrics)) {
		if (metrics) {
			metricFlavor->setVal("json");
		} else {
			metricFlavor->setVal("");
		}
	}
	ImGui::TooltipText("Send anonymous usage statistics");
}

bool MenuBar::update(ui::IMGUIApp *app, command::CommandExecutionListener &listener) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginMenu(ICON_LC_FILE " File")) {
			actionMenuItem(ICON_LC_SQUARE " New", "new", listener);
			actionMenuItem(ICON_LC_FILE_INPUT " Load", "load", listener);
			if (ImGui::BeginMenu(ICON_LC_FILE_STACK " Recently opened")) {
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

			actionMenuItem(ICON_LC_SAVE " Save", "save", listener);
			actionMenuItem(ICON_LC_SAVE " Save as", "saveas", listener);
			ImGui::CommandMenuItem(ICON_LC_FILE " Save selection", "exportselection", !sceneMgr().modifier().selections().empty(), &listener);
			ImGui::Separator();

			actionMenuItem(ICON_LC_PLUS_SQUARE " Add file to scene", "import", listener);
			actionMenuItem(ICON_LC_PLUS_SQUARE " Add directory to scene", "importdirectory", listener);
			ImGui::Separator();
			actionMenuItem(ICON_LC_IMAGE " Heightmap", "importheightmap", listener);
			actionMenuItem(ICON_LC_IMAGE " Colored heightmap", "importcoloredheightmap", listener);
			actionMenuItem(ICON_LC_IMAGE " Image as plane", "importplane", listener);
			actionMenuItem(ICON_LC_IMAGE " Image as volume", "importvolume", listener);
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_LC_DOOR_CLOSED " Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_LC_MENU " Edit")) {
			const SceneManager &sceneManager = sceneMgr();
			const MementoHandler &mementoHandler = sceneManager.mementoHandler();
			ImGui::CommandMenuItem(ICON_LC_ROTATE_CCW " Undo", "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandMenuItem(ICON_LC_ROTATE_CW " Redo", "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			const Modifier &modifier = sceneManager.modifier();
			const Selections &selections = modifier.selections();
			ImGui::CommandMenuItem(ICON_LC_SCISSORS " Cut", "cut", !selections.empty(), &listener);
			ImGui::CommandMenuItem(ICON_LC_COPY " Copy", "copy", !selections.empty(), &listener);
			ImGui::CommandMenuItem(ICON_LC_CLIPBOARD_PASTE " Paste at reference##pastereferencepos", "paste",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandMenuItem(ICON_LC_CLIPBOARD_PASTE " Paste at cursor##pastecursor", "pastecursor",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandMenuItem(ICON_LC_CLIPBOARD_PASTE " Paste as new node##pastenewnode", "pastenewnode",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::Separator();
			if (ImGui::BeginMenu(ICON_LC_MENU " Options")) {
				ImGui::CheckboxVar(ICON_LC_GRID_3X3 " Grid", cfg::VoxEditShowgrid);
				ImGui::CheckboxVar("Show gizmo", cfg::VoxEditShowaxis);
				ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
				ImGui::CheckboxVar(ICON_LC_BOX " Bounding box", cfg::VoxEditShowaabb);
				ImGui::CheckboxVar(ICON_LC_BONE " Bones", cfg::VoxEditShowBones);
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

				metricOption();

				ImGui::ComboVar("Mesh mode", cfg::VoxelMeshMode, {"Cubes", "Marching cubes"});
				ImGui::InputVarInt("Model animation speed", cfg::VoxEditAnimationSpeed);
				ImGui::InputVarInt("Autosave delay in seconds", cfg::VoxEditAutoSaveSeconds);
				ImGui::InputVarInt("Viewports", cfg::VoxEditViewports, 1, 1);
				ImGui::SliderVarFloat("Zoom speed", cfg::ClientCameraZoomSpeed, 0.1f, 200.0f);
				ImGui::SliderVarInt("View distance", cfg::VoxEditViewdistance, 10, 5000);
				ImGui::InputVarInt("Font size", cfg::UIFontSize, 1, 5);

				ImGui::ComboVar("Color theme", cfg::UIStyle, {"CorporateGrey", "Dark", "Light", "Classic"});
				colorReductionOptions();

				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::ButtonFullWidth("Scene settings")) {
				_popupSceneSettings = true;
			}
			if (ImGui::ButtonFullWidth("Bindings")) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_LC_SQUARE " Select")) {
			actionMenuItem("None", "select none", listener);
			actionMenuItem("Invert", "select invert", listener);
			actionMenuItem("All", "select all", listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_LC_HELP_CIRCLE " Help")) {
#ifdef DEBUG
			if (ImGui::BeginMenu(ICON_LC_BUG " Debug")) {
				if (ImGui::Button("Textures")) {
					app->showTexturesDialog();
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::MenuItem("Tip of the day")) {
				_popupTipOfTheDay = true;
			}
			if (ImGui::MenuItem("Welcome screen")) {
				_popupWelcome = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_LC_INFO " About")) {
				_popupAbout = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxedit
