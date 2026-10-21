/**
 * @file
 */

#include "SceneSettingsPanel.h"
#include "SunAngleSettings.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "ui/ScopedStyle.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

SceneSettingsPanel::SceneSettingsPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
	: Super(app, "scenesettings"), _sceneMgr(sceneMgr) {
}

void SceneSettingsPanel::init() {
	_rendershadow = core::getVar(cfg::VoxEditRendershadow);
	_ambientColor = core::getVar(cfg::VoxEditAmbientColor);
	_diffuseColor = core::getVar(cfg::VoxEditDiffuseColor);
	_sunAngle = core::getVar(cfg::VoxEditSunAngle);
	_shadingMode = core::getVar(cfg::VoxEditShadingMode);
}

void SceneSettingsPanel::sceneColors(ShadingMode shadingMode) {
	const bool lightingEnabled = (shadingMode != ShadingMode::Unlit);

	ui::ScopedStyle style;
	if (!lightingEnabled) {
		style.setAlpha(ImGui::GetStyle().Alpha * 0.5f);
	}

	ImGui::ColorEdit3Var(cfg::VoxEditAmbientColor);
	if (lightingEnabled) {
		ImGui::SetItemTooltipUnformatted(_("Base lighting that affects all surfaces equally"));
	} else {
		ImGui::SetItemTooltipUnformatted(_("Ambient color is disabled in Unlit mode"));
	}

	ImGui::ColorEdit3Var(cfg::VoxEditDiffuseColor);
	if (lightingEnabled) {
		ImGui::SetItemTooltipUnformatted(_("Directional lighting that varies based on surface angle"));
	} else {
		ImGui::SetItemTooltipUnformatted(_("Diffuse color is disabled in Unlit mode"));
	}
}

void SceneSettingsPanel::sceneShadowAndSun(ShadingMode shadingMode) {
	if (sunAngleSettings(_sunAngle, shadingMode == ShadingMode::Shadows))
		_sceneMgr->markDirty();
	sceneSunSettings(*_sceneMgr.get());
}

void SceneSettingsPanel::update(const char *id, command::CommandExecutionListener &listener) {
	static ui::ScopedPanel panel(cfg::VoxEditShowSceneSettings);
	if (!panel.isOpen()) {
		return;
	}
	const core::String &title = makeTitle(ICON_LC_SPOTLIGHT, _("Scene settings"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::TextUnformatted(_("Scene settings"));
		ImGui::Separator();

		const char *shadingModeItems[] = {_("Unlit (Pure Colors)"), _("Lit (No Shadows)"), _("Shadows")};
		int currentShadingMode = _shadingMode->intVal();
		ShadingMode shadingMode = (ShadingMode)currentShadingMode;

		if (ImGui::Combo(_("Shading Mode"), &currentShadingMode, shadingModeItems, lengthof(shadingModeItems))) {
			_shadingMode->setVal(currentShadingMode);
			_sceneMgr->markDirty();
			shadingMode = (ShadingMode)currentShadingMode;

			switch (shadingMode) {
			case ShadingMode::Unlit:
				_rendershadow->setVal(false);
				_ambientColor->setVal("1.0 1.0 1.0");
				_diffuseColor->setVal("0.0 0.0 0.0");
				break;
			case ShadingMode::Lit:
				_rendershadow->setVal(false);
				_ambientColor->setVal("0.3 0.3 0.3");
				_diffuseColor->setVal("0.7 0.7 0.7");
				break;
			case ShadingMode::Shadows:
				_rendershadow->setVal(true);
				_ambientColor->setVal("0.3 0.3 0.3");
				_diffuseColor->setVal("0.7 0.7 0.7");
				break;
			}
		}

		sceneColors(shadingMode);
		sceneShadowAndSun(shadingMode);
	}
}

} // namespace voxedit
