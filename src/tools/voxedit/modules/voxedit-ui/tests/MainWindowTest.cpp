/**
 * @file
 */

#include "../MainWindow.h"
#include "../Viewport.h"
#include "../WindowTitles.h"
#include "TestUtil.h"
#include "core/StringUtil.h"
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
	_optionsPanel.registerUITests(engine, TITLE_OPTIONS);
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
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();

		IM_CHECK(isPopupOpen(POPUP_TITLE_UNSAVED));
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

	IM_REGISTER_TEST(engine, testCategory(), "record start")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Record/Start recording");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "recording.vrec"));
		IM_CHECK(_sceneMgr->isRecording());
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(1, 1, 1), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(2, 2, 2), voxel::createVoxel(voxel::VoxelType::Generic, 2)));
		const core::String recordingFile = _sceneMgr->recorder().filename();
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Record/Stop recording");
		ctx->Yield();
		IM_CHECK(!_sceneMgr->isRecording());

		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));

		// playback the recorded file
		IM_CHECK(_sceneMgr->startPlayback(recordingFile));
		IM_CHECK(_sceneMgr->isPlaying());
		// process frames until playback finishes or we've waited long enough
		for (int i = 0; i < 60 && _sceneMgr->isPlaying(); ++i) {
			ctx->Yield();
		}
		IM_CHECK(!_sceneMgr->isPlaying());
		const int activeNode2 = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model2 = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model2 != nullptr);
		IM_CHECK(model2->volume()->voxel(glm::ivec3(1, 1, 1)).getColor() == 1);
		IM_CHECK(model2->volume()->voxel(glm::ivec3(2, 2, 2)).getColor() == 2);
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

	IM_REGISTER_TEST(engine, testCategory(), "file save")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Save");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "file load dialog")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Load");
		ctx->Yield();
		// the load dialog opens the "Select a file" popup
		IM_CHECK(focusWindow(ctx, "Select a file"));
		ctx->ItemClick("###Cancel");
	};

	IM_REGISTER_TEST(engine, testCategory(), "file save as")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Save as");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "uitest.vengi"));
		const core::String suggestedFilename = _sceneMgr->getSuggestedFilename();
		IM_CHECK(core::string::endsWith(suggestedFilename, "uitest.vengi"));
		// load the saved file and verify the name is still correct
		IM_CHECK(_sceneMgr->newScene(true, "empty", new voxel::RawVolume({0, 1})));
		io::FileDescription fd;
		fd.set(suggestedFilename);
		IM_CHECK(_sceneMgr->load(fd));
		for (int i = 0; i < 20 && _sceneMgr->isLoading(); ++i) {
			ctx->Yield();
		}
		IM_CHECK(!_sceneMgr->isLoading());
		IM_CHECK_STR_EQ(_sceneMgr->getSuggestedFilename().c_str(), suggestedFilename.c_str());
	};
}

} // namespace voxedit
