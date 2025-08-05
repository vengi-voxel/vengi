/**
 * @file
 */

#include "../CameraPanel.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void CameraPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create camera node")->TestFunc = [=](ImGuiTestContext *ctx) {
		const size_t beforeCamera = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemInputValue("##camera_props/Position/$$0", 0.0f);
		ctx->ItemInputValue("##camera_props/Position/$$1", 1.0f);
		ctx->ItemInputValue("##camera_props/Position/$$2", 2.0f);
		ctx->ItemClick("toolbar/###button1");
		const size_t afterCamera = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK_EQ(beforeCamera + 1, afterCamera);
	};
}

} // namespace voxedit
