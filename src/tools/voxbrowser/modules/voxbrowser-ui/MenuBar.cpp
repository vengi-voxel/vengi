/**
 * @file
 */

#include "MenuBar.h"
#include "IMGUIApp.h"
#include "ui/IconsLucide.h"

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

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxbrowser
