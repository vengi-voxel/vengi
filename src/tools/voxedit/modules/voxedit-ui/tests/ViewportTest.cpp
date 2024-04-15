/**
 * @file
 */

#include "../Viewport.h"
#include "command/CommandHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	IM_REGISTER_TEST(engine, testCategory(), "set voxel")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (isSceneMode()) {
			return;
		}
		IM_CHECK(focusWindow(ctx, _uiId.c_str()));
		ImGuiWindow* window = ImGui::FindWindowByName(_uiId.c_str());
		IM_CHECK_SILENT(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
	};

	// TODO: a scene mode test to create another node and select a node
	// TODO: copy and paste
}

} // namespace voxedit
