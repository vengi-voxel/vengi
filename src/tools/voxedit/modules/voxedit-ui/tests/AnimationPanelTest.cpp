/**
 * @file
 */

#include "../AnimationPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void AnimationPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "create, select and delete animation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
		const size_t animations = _sceneMgr->sceneGraph().animations().size();
		ctx->ItemInputValue("##nameanimationpanel", "Automated ui test animation");
		ctx->ItemClick("###Add");
		IM_CHECK_EQ(_sceneMgr->sceneGraph().animations().size(), animations + 1);
		ctx->ItemClick("Animation");
		ctx->ItemClick("//$FOCUSED/Automated ui test animation");
		ctx->ItemClick("###Delete");
		IM_CHECK_EQ(_sceneMgr->sceneGraph().animations().size(), animations);
	};
}

} // namespace voxedit
