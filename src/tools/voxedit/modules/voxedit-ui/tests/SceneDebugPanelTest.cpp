/**
 * @file
 */

#include "../SceneDebugPanel.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void SceneDebugPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "stats display")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
	};

	IM_REGISTER_TEST(engine, testCategory(), "cull checkboxes")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		core::VarPtr cullNodes = core::getVar(cfg::RenderCullNodes);
		const bool beforeNodes = cullNodes->boolVal();
		ctx->ItemClick("Cull nodes");
		IM_CHECK(cullNodes->boolVal() != beforeNodes);
		ctx->ItemClick("Cull nodes");
		IM_CHECK(cullNodes->boolVal() == beforeNodes);

		core::VarPtr cullBuffers = core::getVar(cfg::RenderCullBuffers);
		const bool beforeBuffers = cullBuffers->boolVal();
		ctx->ItemClick("Cull buffers");
		IM_CHECK(cullBuffers->boolVal() != beforeBuffers);
		ctx->ItemClick("Cull buffers");
		IM_CHECK(cullBuffers->boolVal() == beforeBuffers);
	};
}

} // namespace voxedit
