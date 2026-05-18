/**
 * @file
 */

#include "BrushPanelPlane.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

void BrushPanelPlane::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	if (modifier.isMode(ModifierType::Place)) {
		ImGui::TextWrappedUnformatted(_("Extrude voxels"));
	} else if (modifier.isMode(ModifierType::Erase)) {
		ImGui::TextWrappedUnformatted(_("Erase voxels"));
	} else if (modifier.isMode(ModifierType::Override)) {
		ImGui::TextWrappedUnformatted(_("Override voxels"));
	}
}

} // namespace voxedit
