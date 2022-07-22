/**
 * @file
 */

#include "MenuBar.h"
#include "core/GameConfig.h"
#include "imgui.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "engine-config.h"

namespace voxedit {

bool MenuBar::actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener) {
	return ImGui::CommandMenuItem(title, command, true, &listener);
}

bool MenuBar::update(ui::imgui::IMGUIApp* app, command::CommandExecutionListener &listener) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
			actionMenuItem("New", "new", listener);
			actionMenuItem(ICON_FK_FLOPPY_O " Load", "load", listener);
			if (ImGui::BeginMenu("Recently opened")) {
				int recentlyOpened = 0;
				for (const core::String& f : _lastOpenedFiles) {
					if (f.empty()) {
						break;
					}
					const core::String& item = core::string::format("%s##%i", f.c_str(), recentlyOpened);
					if (ImGui::MenuItem(item.c_str())) {
						command::executeCommands("load \"" + f + "\"", &listener);
					}
					++recentlyOpened;
				}
				ImGui::EndMenu();
			}

			actionMenuItem(ICON_FA_SAVE " Save", "save", listener);
			actionMenuItem(ICON_FA_SAVE " Save as", "saveas", listener);
			actionMenuItem(ICON_FA_CAMERA " Screenshot", "screenshot", listener);
			ImGui::Separator();
			actionMenuItem("Load Animation", "animation_load", listener);
			actionMenuItem(ICON_FA_SAVE " Save Animation", "animation_save", listener);
			ImGui::Separator();
			actionMenuItem("Prefab", "prefab", listener);
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE " Heightmap", "importheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as plane", "importplane", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as volume", "importvolume", listener);
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_COG " Edit")) {
			const SceneManager &sceneManager = sceneMgr();
			const MementoHandler &mementoHandler = sceneManager.mementoHandler();
			ImGui::CommandMenuItem(ICON_FA_UNDO " Undo", "undo", mementoHandler.canUndo(), &listener);
			ImGui::CommandMenuItem(ICON_FA_REDO " Redo", "redo", mementoHandler.canRedo(), &listener);
			ImGui::Separator();
			const Modifier &modifier = sceneManager.modifier();
			const Selection &selection = modifier.selection();
			ImGui::CommandMenuItem(ICON_FA_CUT " Cut", "cut", selection.isValid(), &listener);
			ImGui::CommandMenuItem(ICON_FA_COPY " Copy", "copy", selection.isValid(), &listener);
			ImGui::CommandMenuItem(ICON_FA_PASTE " Paste", "paste", sceneManager.hasClipboardCopy(), &listener);
			ImGui::Separator();
			if (ImGui::BeginMenu(ICON_FA_COG " Options")) {
				ImGui::CheckboxVar(ICON_FA_BORDER_ALL " Grid", cfg::VoxEditShowgrid);
				ImGui::CheckboxVar("Show axis", cfg::VoxEditShowaxis);
				ImGui::CheckboxVar("Model space", cfg::VoxEditModelSpace);
				ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
				ImGui::CheckboxVar(ICON_FA_DICE_SIX " Bounding box", cfg::VoxEditShowaabb);
				ImGui::CheckboxVar("Shadow", cfg::VoxEditRendershadow);
				ImGui::CheckboxVar("Bloom", cfg::ClientBloom);
				ImGui::SliderVarInt("Zoom speed", cfg::VoxEditCameraZoomSpeed, 10, 200);
				ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::Button("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(ICON_FA_EYE " View")) {
				actionMenuItem("Reset camera", "resetcamera", listener);
				actionMenuItem("Scene view", "togglescene", listener);
				actionMenuItem("Console", "toggleconsole", listener);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::Button("Scene settings")) {
				_popupSceneSettings = true;
			}
			if (ImGui::Button("Bindings")) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_SQUARE " Select")) {
			actionMenuItem("None", "select none", listener);
			actionMenuItem("All", "select all", listener);
			ImGui::EndMenu();
		}
#ifdef DEBUG
		if (ImGui::BeginMenu(ICON_FK_BUG " Debug")) {
			if (ImGui::Button("Textures")) {
				app->showTexturesDialog();
			}
			ImGui::EndMenu();
		}
#endif
		if (ImGui::BeginMenu(ICON_FK_INFO " About")) {
			ImGui::Text("VoxEdit " PROJECT_VERSION);
			ImGui::Separator();

			ImGui::URLItem(ICON_FK_GITHUB " Bug reports", "https://github.com/mgerhardy/vengi");
			ImGui::URLItem(ICON_FK_TWITTER " Twitter", "https://twitter.com/MartinGerhardy");
			ImGui::URLItem(ICON_FK_DISCORD " Discord", "https://discord.gg/AgjCPXy");

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

}
