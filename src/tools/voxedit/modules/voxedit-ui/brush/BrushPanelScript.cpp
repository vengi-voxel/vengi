/**
 * @file
 */

#include "BrushPanelScript.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/LUABrush.h"
#include "voxelui/LUAScriptParameters.h"

namespace voxedit {

void BrushPanelScript::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();

	ImGui::CommandButton(_("Rescan"), "brushscriptrescan", listener);

	LUABrush *activeBrush = modifier.scriptManager().activeLuaBrush();
	if (activeBrush == nullptr) {
		return;
	}

	if (!activeBrush->scriptDescription().empty()) {
		ImGui::TextWrappedUnformatted(activeBrush->scriptDescription().c_str());
	}

	voxelui::renderScriptParameters(activeBrush->parameterDescriptions(), activeBrush->parameters());
}

} // namespace voxedit
