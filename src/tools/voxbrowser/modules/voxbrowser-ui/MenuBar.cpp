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

bool MenuBar::update() {
	bool resetDockLayout = false;
	if (ImGui::BeginMenuBar()) {
		core_trace_scoped(MenuBar);
		if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
			ImGui::CommandIconMenuItem(ICON_LC_DOWNLOAD, _("Download missing files"), "downloadall");
			ImGui::CommandIconMenuItem(ICON_LC_IMAGE, _("Download missing thumbnails"), "thumbnaildownloadall");
			ImGui::Separator();
			if (ImGui::IconMenuItem(ICON_LC_DOOR_CLOSED, _("Quit"))) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Edit"))) {
			if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
				ImGui::BeginDisabled(core::Var::get(cfg::VoxelMeshMode)->intVal() != (int)voxel::SurfaceExtractionType::Cubic);
				ImGui::CheckboxVar(_("Outlines"), cfg::RenderOutline);
				ImGui::EndDisabled();
				ImGui::CheckboxVar(_("Bloom"), cfg::ClientBloom);
				ui::metricOption();
				ImGui::CheckboxVar(_("Allow multi monitor"), cfg::UIMultiMonitor);
				ImGui::InputVarInt(_("Font size"), cfg::UIFontSize, 1, 5);
				const core::Array<core::String, ImGui::MaxStyles> uiStyles = {_("CorporateGrey"), _("Dark"), _("Light"),
																			  _("Classic")};
				ImGui::ComboVar(_("Color theme"), cfg::UIStyle, uiStyles);
				ImGui::InputVarFloat(_("Notifications"), cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth(_("Reset layout"))) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginIconMenu(ICON_LC_HELP_CIRCLE, _("Help"))) {
#ifdef DEBUG
			if (ImGui::BeginIconMenu(ICON_LC_BUG, _("Debug"))) {
				if (ImGui::Button(_("Textures"))) {
					_app->showTexturesDialog();
				}
				ImGui::EndMenu();
			}
#endif
			if (ImGui::MenuItem(_("Show all commands"))) {
				_app->showCommandDialog();
			}
			if (ImGui::MenuItem(_("Show all cvars"))) {
				_app->showCvarDialog();
			}
			if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
				_popupAbout = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxbrowser
