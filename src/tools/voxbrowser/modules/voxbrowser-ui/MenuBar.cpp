/**
 * @file
 */

#include "MenuBar.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxel/SurfaceExtractor.h"

namespace voxbrowser {

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
				metricOption();
				ImGui::CheckboxVar("Allow multi monitor", cfg::UIMultiMonitor);
				ImGui::InputVarInt("Font size", cfg::UIFontSize, 1, 5);
				ImGui::ComboVar("Color theme", cfg::UIStyle, {"CorporateGrey", "Dark", "Light", "Classic"});
				ImGui::InputVarFloat("Notifications", cfg::UINotifyDismissMillis);
				if (ImGui::ButtonFullWidth("Reset layout")) {
					resetDockLayout = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	return resetDockLayout;
}

} // namespace voxbrowser
