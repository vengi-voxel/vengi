/**
 * @file
 */

#include "../NetworkPanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void NetworkPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "tab switching")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->ItemClick("##networktabbar/Client");
		ctx->Yield();
		ctx->ItemClick("##networktabbar/Server");
		ctx->Yield();
		ctx->ItemClick("##networktabbar/Client");
		ctx->Yield();
	};
}

} // namespace voxedit
