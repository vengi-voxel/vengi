/**
 * @file
 */

#include "../MainWindow.h"
#include "../WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void MainWindow::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if ENABLE_RENDER_PANEL
	_renderPanel.registerUITests(engine, TITLE_RENDER);
#endif
	_lsystemPanel.registerUITests(engine, TITLE_LSYSTEMPANEL);
	_brushPanel.registerUITests(engine, TITLE_BRUSHPANEL);
	_treePanel.registerUITests(engine, TITLE_TREES);
	_sceneGraphPanel.registerUITests(engine, TITLE_SCENEGRAPH);
	_animationPanel.registerUITests(engine, TITLE_ANIMATION_SETTINGS);
	_toolsPanel.registerUITests(engine, TITLE_TOOLS);
	_assetPanel.registerUITests(engine, TITLE_ASSET);
	_mementoPanel.registerUITests(engine, TITLE_MEMENTO);
	_positionsPanel.registerUITests(engine, TITLE_POSITIONS);
	_palettePanel.registerUITests(engine, TITLE_PALETTE);
	_menuBar.registerUITests(engine, nullptr);
	_statusBar.registerUITests(engine, TITLE_STATUSBAR);
	_scriptPanel.registerUITests(engine, TITLE_SCRIPT_EDITOR);
	_animationTimeline.registerUITests(engine, TITLE_ANIMATION_TIMELINE);
	_cameraPanel.registerUITests(engine, TITLE_CAMERA);

	IM_REGISTER_TEST(engine, testName(), "new scene")->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		focusWindow(ctx, title);
		ctx->MenuClick("###File/###New");
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###OK");
	};
}

} // namespace voxedit
