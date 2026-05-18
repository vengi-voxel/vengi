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
		ImGui::TextWrappedUnformatted(_("Fill the clicked face, or drag to extrude the plane"));
	} else if (modifier.isMode(ModifierType::Erase)) {
		ImGui::TextWrappedUnformatted(_("Erase voxels on the clicked face, or drag to extrude"));
	} else if (modifier.isMode(ModifierType::Override)) {
		ImGui::TextWrappedUnformatted(_("Recolor voxels on the clicked face, or drag to extrude"));
	}
}

} // namespace voxedit
