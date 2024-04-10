/**
 * @file
 */

#include "../AnimationPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void AnimationPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	ImGuiTest *createAnim = IM_REGISTER_TEST(engine, testName(), "create, select and delete animation");
	createAnim->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		focusWindow(ctx, title);
		const size_t animations = _sceneMgr->sceneGraph().animations().size();
		ctx->ItemInputValue("##nameanimationpanel", "Automated ui test animation");
		ctx->ItemClick("###Add");
		IM_CHECK_EQ(_sceneMgr->sceneGraph().animations().size(), animations + 1);
		// TODO: select and delete
	};
}

} // namespace voxedit
