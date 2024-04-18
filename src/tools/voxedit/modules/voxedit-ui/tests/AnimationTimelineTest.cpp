/**
 * @file
 */

#include "../AnimationTimeline.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void AnimationTimeline::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if 0
	// https://gitlab.com/GroGy/im-neo-sequencer/-/issues/28
	IM_REGISTER_TEST(engine, testCategory(), "create keyframe")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
		ctx->MouseMove("//##neo-sequencer/##_top_selector_neo");
		ctx->MouseDragWithDelta({10.0f, 0.0f});
		ctx->ItemClick("###Add");
	};
#endif
}

} // namespace voxedit
