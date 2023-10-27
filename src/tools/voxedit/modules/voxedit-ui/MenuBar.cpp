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
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"

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

bool MenuBar::update(ui::IMGUIApp *app, command::CommandExecutionListener &listener) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
			actionMenuItem(ICON_FA_SQUARE " New", "new", listener);
			actionMenuItem(ICON_FK_FLOPPY_O " Load", "load", listener);
			if (ImGui::BeginMenu(ICON_FA_BARS " Recently opened")) {
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

			actionMenuItem(ICON_FA_FLOPPY_DISK " Save", "save", listener);
			actionMenuItem(ICON_FA_FLOPPY_DISK " Save as", "saveas", listener);
			ImGui::CommandMenuItem(ICON_FA_FILE " Save selection", "exportselection", !sceneMgr().modifier().selections().empty(), &listener);
			ImGui::Separator();

			actionMenuItem(ICON_FA_SQUARE_PLUS " Add file to scene", "import", listener);
			actionMenuItem(ICON_FA_SQUARE_PLUS " Add directory to scene", "importdirectory", listener);
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE " Heightmap", "importheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Colored heightmap", "importcoloredheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as plane", "importplane", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as volume", "importvolume", listener);
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_DOOR_CLOSED " Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_GEAR " Edit")) {
			const SceneManager &sceneManager = sceneMgr();
			const MementoHandler &mementoHandler = sceneManager.mementoHandler();
			ImGui::CommandMenuItem(ICON_FA_ROTATE_LEFT " Undo", "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandMenuItem(ICON_FA_ROTATE_RIGHT " Redo", "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			const Modifier &modifier = sceneManager.modifier();
			const Selections &selections = modifier.selections();
			ImGui::CommandMenuItem(ICON_FA_SCISSORS " Cut", "cut", !selections.empty(), &listener);
			ImGui::CommandMenuItem(ICON_FA_COPY " Copy", "copy", !selections.empty(), &listener);
			ImGui::CommandMenuItem(ICON_FA_PASTE " Paste at reference##pastereferencepos", "paste",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandMenuItem(ICON_FA_PASTE " Paste at cursor##pastecursor", "pastecursor",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::CommandMenuItem(ICON_FA_PASTE " Paste as new node##pastenewnode", "pastenewnode",
								   sceneManager.hasClipboardCopy(), &listener);
			ImGui::Separator();
			actionMenuItem(ICON_FK_TERMINAL " Console", "toggleconsole", listener);
			ImGui::Separator();
			if (ImGui::BeginMenu(ICON_FA_GEAR " Options")) {
				ImGui::CheckboxVar(ICON_FA_BORDER_ALL " Grid", cfg::VoxEditShowgrid);
				ImGui::CheckboxVar("Show gizmo", cfg::VoxEditShowaxis);
				ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
				ImGui::CheckboxVar(ICON_FA_DICE_SIX " Bounding box", cfg::VoxEditShowaabb);
				ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() == 1);
				ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
				ImGui::EndDisabled();
				ImGui::CheckboxVar("Shadow", cfg::VoxEditRendershadow);
				ImGui::CheckboxVar("Bloom", cfg::ClientBloom);
				ImGui::CheckboxVar("Allow multi monitor", cfg::UIMultiMonitor);
				ImGui::CheckboxVar("Color picker", cfg::VoxEditShowColorPicker);
				ImGui::CheckboxVar("Color wheel", cfg::VoxEditColorWheel);
				ImGui::CheckboxVar("Simplified UI", cfg::VoxEditSimplifiedView);
				ImGui::CheckboxVar("Tip of the day", cfg::VoxEditTipOftheDay);

				const core::VarPtr &metricFlavor = core::Var::getSafe(cfg::MetricFlavor);
				bool metrics = !metricFlavor->strVal().empty();
				if (ImGui::Checkbox("Enable sending metrics", &metrics)) {
					if (metrics) {
						metricFlavor->setVal("json");
					} else {
						metricFlavor->setVal("");
					}
				}
				ImGui::TooltipText("Send anonymous usage statistics");

				// TODO: activate me - the RawVolumeRenderer doesn't yet update the shader attributes correctly when switching between shaders
				// ImGui::ComboVar("Mesh mode", cfg::VoxelMeshMode, {"Cubes", "Marching cubes"});
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
		if (ImGui::BeginMenu(ICON_FA_SQUARE " Select")) {
			actionMenuItem("None", "select none", listener);
			actionMenuItem("Invert", "select invert", listener);
			actionMenuItem("All", "select all", listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FK_QUESTION " Help")) {
#ifdef DEBUG
			if (ImGui::BeginMenu(ICON_FK_BUG " Debug")) {
				if (ImGui::Button("Textures")) {
					app->showTexturesDialog();
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::MenuItem("Tip of the day")) {
				_popupTipOfTheDay = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FK_INFO " About")) {
				_popupAbout = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxedit
