/**
 * @file
 */

#include "BrushPanelNormal.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/NormalBrush.h"

namespace voxedit {

void BrushPanelNormal::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	NormalBrush &brush = modifier.normalBrush();
	if (!ctx.renderNormals->boolVal()) {
		ImGui::TextWrappedUnformatted(_("Enable normal rendering to see your changes"));
	}
	NormalBrush::PaintMode paintMode = brush.paintMode();
	int paintModeInt = (int)paintMode;
	if (ImGui::Combo(_("Mode"), &paintModeInt, NormalBrush::PaintModeStr, (int)NormalBrush::PaintMode::Max)) {
		brush.setPaintMode((NormalBrush::PaintMode)paintModeInt);
	}
}

} // namespace voxedit
