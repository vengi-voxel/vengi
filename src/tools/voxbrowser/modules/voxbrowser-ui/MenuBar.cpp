/**
 * @file
 */

#include "MenuBar.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "IMGUIStyle.h"
#include "PopupAbout.h"
#include "ui/IconsLucide.h"
#include "voxel/SurfaceExtractor.h"

namespace voxbrowser {

bool MenuBar::update(ui::IMGUIApp *app) {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginMenu(ICON_LC_FILE " File")) {
			// actionMenuItem(ICON_LC_SAVE " Save");
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_LC_DOOR_CLOSED " Quit")) {
				app->requestQuit();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_LC_MENU " Edit")) {
			if (ImGui::BeginMenu(ICON_LC_MENU " Options")) {
				ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() != (int)voxel::SurfaceExtractionType::Cubic);
				ImGui::CheckboxVar("Outlines", cfg::RenderOutline);
				ImGui::EndDisabled();
				ImGui::CheckboxVar("Bloom", cfg::ClientBloom);
				ui::metricOption();
				ImGui::CheckboxVar("Allow multi monitor", cfg::UIMultiMonitor);
				ImGui::InputVarInt("Font size", cfg::UIFontSize, 1, 5);
				static const core::Array<core::String, ImGui::MaxStyles> uiStyles = {"CorporateGrey", "Dark", "Light",
																					 "Classic"};
				ImGui::ComboVar("Color theme", cfg::UIStyle, uiStyles);
				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
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
			if (ImGui::MenuItem(ICON_LC_INFO " About")) {
				_popupAbout = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxbrowser
