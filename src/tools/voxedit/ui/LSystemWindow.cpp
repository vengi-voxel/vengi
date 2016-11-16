/**
 * @file
 */

#include "LSystemWindow.h"
#include "voxel/generator/LSystemGenerator.h"

namespace voxedit {

namespace {
voxel::lsystem::LSystemContext ctx;
}

LSystemWindow::LSystemWindow(ui::Window* window) :
		Super(window) {
	core_assert_always(loadResourceFile("ui/window/voxedit-lsystem.tb.txt"));
}

bool LSystemWindow::OnEvent(const tb::TBWidgetEvent &ev) {
/*
	tb::TBEditField* axiom = getWidgetByType<tb::TBEditField>("lsystem_axiom");
	if (axiom != nullptr) {
		const tb::TBStr& axiomStr = ev.target->GetText();
		Log::info("execute lsystem %s", axiomStr.CStr());
		ctx.axiom = axiomStr.CStr();
		ctx.productionRules.emplace('A', ctx.axiom);
		ctx.voxels.emplace('A', voxel::createVoxel(_paletteWidget->voxelType()));
		ctx.generations = 2;
		ctx.start = _scene->cursorPosition();
		_scene->lsystem(ctx);
	}
	*/
	return Super::OnEvent(ev);
}

}
