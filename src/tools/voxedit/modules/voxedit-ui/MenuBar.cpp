/**
 * @file
 */

#include "MenuBar.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "engine-config.h"

namespace voxedit {

bool MenuBar::actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener) {
	return ImGui::CommandMenuItem(title, command, true, &listener);
}

void MenuBar::update(ui::imgui::IMGUIApp* app, command::CommandExecutionListener &listener) {
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
			actionMenuItem("New", "new", listener);
			actionMenuItem(ICON_FK_FLOPPY_O " Load", "load", listener);
			actionMenuItem(ICON_FA_SAVE " Save", "save", listener);
			actionMenuItem(ICON_FA_CAMERA " Screenshot", "screenshot", listener);
			ImGui::Separator();
			actionMenuItem("Load Animation", "animation_load", listener);
			actionMenuItem(ICON_FA_SAVE " Save Animation", "animation_save", listener);
			ImGui::Separator();
			actionMenuItem("Prefab", "prefab", listener);
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE " Heightmap", "importheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as Plane", "importplane", listener);
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}
		ImGui::CommandMenuItem(ICON_FA_UNDO " Undo", "undo", sceneMgr().mementoHandler().canUndo(), &listener);
		ImGui::CommandMenuItem(ICON_FA_REDO " Redo", "redo", sceneMgr().mementoHandler().canRedo(), &listener);
		if (ImGui::BeginMenu(ICON_FA_COG " Options")) {
			ImGui::CheckboxVar(ICON_FA_BORDER_ALL " Grid", cfg::VoxEditShowgrid);
			ImGui::CheckboxVar("Show axis", cfg::VoxEditShowaxis);
			ImGui::CheckboxVar("Model space", cfg::VoxEditModelSpace);
			ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
			ImGui::CheckboxVar(ICON_FA_DICE_SIX " Bounding box", cfg::VoxEditShowaabb);
			ImGui::CheckboxVar("Shadow", cfg::VoxEditRendershadow);
			ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
			if (ImGui::Button("Scene settings")) {
				_popupSceneSettings = true;
			}
			if (ImGui::Button("Bindings")) {
				app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_EYE " View")) {
			actionMenuItem("Reset camera", "resetcamera", listener);
			actionMenuItem("Scene view", "togglescene", listener);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FK_INFO " About")) {
			ImGui::Text("VoxEdit " PROJECT_VERSION);
			ImGui::Separator();

			ImGui::URLItem(ICON_FK_GITHUB " Bug reports", "https://github.com/mgerhardy/engine");
			ImGui::URLItem(ICON_FK_TWITTER " Twitter", "https://twitter.com/MartinGerhardy");
			ImGui::URLItem(ICON_FK_DISCORD " Discord", "https://discord.gg/AgjCPXy");

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

}
