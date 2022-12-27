/**
 * @file
 */

#include "MenuBar.h"
#include "core/Color.h"
#include "core/GameConfig.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsForkAwesome.h"
#include "ui/IconsFontAwesome6.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "engine-config.h"

namespace voxedit {

bool MenuBar::actionMenuItem(const char *title, const char *command, command::CommandExecutionListener &listener) {
	return ImGui::CommandMenuItem(title, command, true, &listener);
}

bool MenuBar::update(ui::IMGUIApp* app, command::CommandExecutionListener &listener) {
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

			actionMenuItem(ICON_FA_FLOPPY_DISK " Save", "save", listener);
			actionMenuItem(ICON_FA_FLOPPY_DISK " Save as", "saveas", listener);
			actionMenuItem(ICON_FA_CAMERA " Screenshot", "screenshot", listener);
			ImGui::Separator();
			actionMenuItem("Prefab", "prefab", listener);
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE " Heightmap", "importheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Colored heightmap", "importcoloredheightmap", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as plane", "importplane", listener);
			actionMenuItem(ICON_FA_IMAGE " Image as volume", "importvolume", listener);
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
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
			const Selection &selection = modifier.selection();
			ImGui::CommandMenuItem(ICON_FA_SCISSORS " Cut", "cut", selection.isValid(), &listener);
			ImGui::CommandMenuItem(ICON_FA_COPY " Copy", "copy", selection.isValid(), &listener);
			ImGui::CommandMenuItem(ICON_FA_PASTE " Paste", "paste", sceneManager.hasClipboardCopy(), &listener);
			ImGui::Separator();
			if (ImGui::BeginMenu(ICON_FA_GEAR " Options")) {
				ImGui::CheckboxVar(ICON_FA_BORDER_ALL " Grid", cfg::VoxEditShowgrid);
				ImGui::CheckboxVar("Show axis", cfg::VoxEditShowaxis);
				ImGui::CheckboxVar("Show locked axis", cfg::VoxEditShowlockedaxis);
				ImGui::CheckboxVar(ICON_FA_DICE_SIX " Bounding box", cfg::VoxEditShowaabb);
				ImGui::CheckboxVar("Shadow", cfg::VoxEditRendershadow);
				ImGui::CheckboxVar("Bloom", cfg::ClientBloom);
				ImGui::CheckboxVar("Color wheel", cfg::VoxEditColorWheel);
				ImGui::SliderVarFloat("Zoom speed", cfg::ClientCameraZoomSpeed, 0.1f, 200.0f);
				ImGui::SliderVarInt("View distance", cfg::VoxEditViewdistance, 10, 5000);
				ImGui::InputVarInt("Font size", cfg::UIFontSize, 1, 5);

				static constexpr const char* ColorThemeStr[] {
					"CorporateGrey",
					"Dark",
					"Light",
					"Classic"
				};
				const core::VarPtr &uistyle = core::Var::getSafe(cfg::UIStyle);
				const int currentUIStyle = uistyle->intVal();
				if (ImGui::BeginCombo("Color theme", ColorThemeStr[(int)currentUIStyle], ImGuiComboFlags_None)) {
					for (int i = 0; i < lengthof(ColorThemeStr); ++i) {
						const bool selected = i == currentUIStyle;
						if (ImGui::Selectable(ColorThemeStr[i], selected)) {
							uistyle->setVal(core::string::toString(i));
						}
						if (selected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				glm::vec3 omega = sceneMgr().activeCamera()->omega();
				if (ImGui::InputFloat("Camera rotation", &omega.y)) {
					sceneMgr().activeCamera()->setOmega(omega);
				}
				ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::Button("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(ICON_FA_EYE " View")) {
				actionMenuItem(ICON_FK_VIDEO_CAMERA " Reset camera", "resetcamera", listener);
				if (ImGui::MenuItem(ICON_FK_TH " View toggle")) {
					// don't use togglescene action menu item - as tab is used for the menu navigation
					// that's the reason why this is bound to editor mode only
					sceneMgr().toggleEditMode();
					listener("togglescene", {});
				}
				actionMenuItem(ICON_FK_TERMINAL " Console", "toggleconsole", listener);
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
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 360.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Green);
			ImGui::TextWrapped("This application is not yet ready for production use. We are always looking for help and feedback to improve things. If you are a developer (C++ and lua) or voxel artist, please consider contributing.");
			ImGui::PopStyleColor(1);
			ImGui::PopTextWrapPos();
			ImGui::URLItem(ICON_FK_GITHUB " Bug reports", "https://github.com/mgerhardy/vengi");
			ImGui::URLItem(ICON_FK_TWITTER " Twitter", "https://twitter.com/MartinGerhardy");
			ImGui::URLItem(ICON_FK_MASTODON " Mastodon", "https://mastodon.social/@mgerhardy");
			ImGui::URLItem(ICON_FK_DISCORD " Discord", "https://discord.gg/AgjCPXy");

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

}
