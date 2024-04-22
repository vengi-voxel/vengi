/**
 * @file
 */

#include "../AnimationTimeline.h"
#include "core/String.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void AnimationTimeline::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if 0
	// https://github.com/ocornut/imgui_test_engine/issues/48
	// https://gitlab.com/GroGy/im-neo-sequencer/-/issues/28
	IM_REGISTER_TEST(engine, testCategory(), "create keyframe")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());

		IM_CHECK(focusWindow(ctx, title));
		ctx->SetRef(ctx->WindowInfo("##sequencer_child_wrapper").ID);
		ctx->MouseMove("sequencer/##_top_selector_neo");
		ctx->MouseDragWithDelta({10.0f, 0.0f});
		ctx->ItemClick("###Add");
	};
#endif
}

} // namespace voxedit
