/**
 * @file
 */

#include "../BrushPanel.h"
#include "ui/IMGUIApp.h"
#include "command/CommandHandler.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void BrushPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "cycle brush types")->TestFunc = [=](ImGuiTestContext *ctx) {
		viewportEditMode(ctx, _app);

		// now we can focus the brush panel
		IM_CHECK(focusWindow(ctx, title));

		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			const core::String id = core::String::format("brushes/###button%d", i);
			ctx->ItemClick(id.c_str());
			ctx->Yield();
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "select")->TestFunc = [=](ImGuiTestContext *ctx) {
		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		modifier.setBrushType(BrushType::None);
		modifier.setModifierType(ModifierType::Select);

		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));

		modifier.unselect();
		IM_CHECK(modifier.selections().empty());

		command::executeCommands("+actionexecute 1 1");
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId, ImVec2(-100, -100)));
		command::executeCommands("-actionexecute 1 1");
		IM_CHECK(!modifier.selections().empty());

		modifier.unselect();
	};
	// TODO: copy and paste
}

} // namespace voxedit
