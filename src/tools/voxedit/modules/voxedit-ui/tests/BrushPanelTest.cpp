/**
 * @file
 */

#include "../BrushPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void BrushPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	ImGuiTest *test = IM_REGISTER_TEST(engine, testCategory(), testName());
	test->TestFunc = [=](ImGuiTestContext *ctx) {
		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		ctx->SetRef(title);
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			ctx->ItemClick(i);
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
		}
	};
}

} // namespace voxedit
