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
	_animationPanel.registerUITests(engine, TITLE_ANIMATION_SETTINGS);
	_animationTimeline.registerUITests(engine, TITLE_ANIMATION_TIMELINE);
	_assetPanel.registerUITests(engine, TITLE_ASSET);
	// collection panel tests belong to the asset panel
	_brushPanel.registerUITests(engine, TITLE_BRUSHPANEL);
	_cameraPanel.registerUITests(engine, TITLE_CAMERA);
	_gameModePanel.registerUITests(engine, TITLE_GAMEMODE);
	_helpPanel.registerUITests(engine, TITLE_HELP);
	_lsystemPanel.registerUITests(engine, TITLE_LSYSTEMPANEL);
	_mementoPanel.registerUITests(engine, TITLE_MEMENTO);
	_menuBar.registerUITests(engine, "##MenuBar");
	_networkPanel.registerUITests(engine, TITLE_NETWORK);
	_nodeInspectorPanel.registerUITests(engine, TITLE_NODE_INSPECTOR);
	_nodePropertiesPanel.registerUITests(engine, TITLE_NODE_PROPERTIES);
	_normalPalettePanel.registerUITests(engine, TITLE_NORMALPALETTE);
	_palettePanel.registerUITests(engine, TITLE_PALETTE);
#if ENABLE_RENDER_PANEL
	_renderPanel.registerUITests(engine, TITLE_RENDER);
#endif
	_sceneGraphPanel.registerUITests(engine, TITLE_SCENEGRAPH);
	_sceneSettingsPanel.registerUITests(engine, TITLE_SCENE_SETTINGS);
	_scriptPanel.registerUITests(engine, TITLE_SCRIPT);
	_statusBar.registerUITests(engine, TITLE_STATUSBAR);
	_toolsPanel.registerUITests(engine, TITLE_TOOLS);
	// viewport tests are registered at init phase

	// main window itself
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

	IM_REGISTER_TEST(engine, testCategory(), "edit options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Edit/Options");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "select menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "selectmenutest", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Select/None");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Select/All");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Select/Invert");
		ctx->Yield();
	};

	// TODO: file dialog load and save
}

} // namespace voxedit
