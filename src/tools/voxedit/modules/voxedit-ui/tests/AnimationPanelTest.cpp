/**
 * @file
 */

#include "../AnimationPanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void AnimationPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create, select and delete animation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		const size_t animations = _sceneMgr->sceneGraph().animations().size();
		ctx->ItemClick("###Add new animation");
		ctx->Yield(2);
		IM_CHECK(focusWindow(ctx, POPUP_TITLE_CREATE_ANIMATION));
		ctx->ItemInputValue("Name", "automated ui test animation");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id)); // back to the original window
		IM_CHECK_EQ(_sceneMgr->sceneGraph().animations().size(), animations + 1);
		ctx->ItemClick("Animation");
		ctx->ItemClick("//$FOCUSED/automated ui test animation");
		ctx->ItemClick("###Delete");
		IM_CHECK_EQ(_sceneMgr->sceneGraph().animations().size(), animations);
	};
}

} // namespace voxedit
