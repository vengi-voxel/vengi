/**
 * @file
 */

#include "../MainWindow.h"
#include "../Viewport.h"
#include "../WindowTitles.h"
#include "TestUtil.h"
#include "ui/PopupAbout.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

void MainWindow::registerUITests(ImGuiTestEngine *engine, const char *id) {
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
	_nodeInspectorPanel.registerUITests(engine, TITLE_NODE_INSPECTOR);
	_nodePropertiesPanel.registerUITests(engine, TITLE_NODE_PROPERTIES);
	_palettePanel.registerUITests(engine, TITLE_PALETTE);
	_normalPalettePanel.registerUITests(engine, TITLE_NORMALPALETTE);
	_menuBar.registerUITests(engine, "##MenuBar");
	_statusBar.registerUITests(engine, TITLE_STATUSBAR);
	_scriptPanel.registerUITests(engine, TITLE_SCRIPT);
	_animationTimeline.registerUITests(engine, TITLE_ANIMATION_TIMELINE);
	_cameraPanel.registerUITests(engine, TITLE_CAMERA);

	IM_REGISTER_TEST(engine, testCategory(), "new scene unsaved changes")->TestFunc = [=](ImGuiTestContext *ctx) {
		_sceneMgr->markDirty();
		ImGuiContext &g = *ctx->UiContext;
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();
		IM_CHECK_EQ(g.OpenPopupStack.Size, 1);
		IM_CHECK_EQ(g.OpenPopupStack[0].PopupId, ctx->GetID(POPUP_TITLE_UNSAVED));
		ctx->SetRef(POPUP_TITLE_UNSAVED);
		ctx->ItemClick("###Yes");
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###Ok");
	};

	IM_REGISTER_TEST(engine, testCategory(), "new scene")->TestFunc = [=](ImGuiTestContext *ctx) {
		_sceneMgr->newScene(true, "", new voxel::RawVolume({0, 1}));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###Ok");
	};

	IM_REGISTER_TEST(engine, testCategory(), "new scene template")->TestFunc = [=](ImGuiTestContext *ctx) {
		_sceneMgr->newScene(true, "", new voxel::RawVolume({0, 1}));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemClick("##templates/##Knight");
		IM_CHECK(_sceneMgr->sceneGraph().findNodeByName("K_Waist") != nullptr);
	};

	IM_REGISTER_TEST(engine, testCategory(), "tip of the day")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Help/Tip of the day");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_TIPOFTHEDAY);
		ctx->ItemClick("###Next");
		ctx->ItemClick("###Next");
		ctx->ItemClick("###Next");
		ctx->ItemClick("###Close");
	};

	IM_REGISTER_TEST(engine, testCategory(), "welcome screen")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Help/Welcome screen");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_WELCOME);
		ctx->ItemClick("###Close");
	};

	IM_REGISTER_TEST(engine, testCategory(), "about screen")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Help/About");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_ABOUT);
		const ImGuiID wrapperId = ctx->WindowInfo("##scrollwindow").ID;
		ctx->SetRef(wrapperId);
		ctx->MouseMove("##abouttabbar/Credits");
		ctx->MouseClick();
		ctx->MouseMove("##abouttabbar/Paths");
		ctx->MouseClick();
		ctx->SetRef(POPUP_TITLE_ABOUT);
		ctx->ItemClick("###Close");
	};

	// TODO: POPUP_TITLE_SCENE_SETTINGS

	// TODO: file dialog load and save
}

} // namespace voxedit
