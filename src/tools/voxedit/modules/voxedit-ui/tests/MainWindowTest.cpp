/**
 * @file
 */

#include "../MainWindow.h"
#include "../Viewport.h"
#include "../WindowTitles.h"
#include "TestUtil.h"
#include "core/StringUtil.h"
#include "ui/PopupAbout.h"
#include "util/KeybindingHandler.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

void MainWindow::registerUITests(ImGuiTestEngine *engine, const char *id) {
	_animationPanel.registerUITests(engine, TITLE_ANIMATION_SETTINGS);
	_animationTimeline.registerUITests(engine, TITLE_ANIMATION_TIMELINE);
	_modelAssetPanel.registerUITests(engine, TITLE_ASSET_MODELS);
	_imageAssetPanel.registerUITests(engine, TITLE_ASSET_IMAGES);
	// collection panel tests belong to the asset panel
	_brushPanel.registerUITests(engine, TITLE_BRUSH_TOOLBAR);
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
	_voxBoxBrowserPanel.registerUITests(engine, TITLE_VOXBOX_BROWSER);
	_sceneDebugPanel.registerUITests(engine, TITLE_SCENEDEBUGPANEL);
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
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_NEW_SCENE);
		ctx->ItemInputValue("##newscenename", "Automated ui test");
		ctx->ItemClick("###Ok");
	};

	IM_REGISTER_TEST(engine, testCategory(), "new scene template")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
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

	IM_REGISTER_TEST(engine, testCategory(), "welcome screen options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Help/Welcome screen");
		ctx->Yield();
		ctx->SetRef(POPUP_TITLE_WELCOME);

		// the view mode combo should exist
		const ImGuiTestItemInfo viewMode = ctx->ItemInfo("View mode", ImGuiTestOpFlags_NoError);
		IM_CHECK(viewMode.ID != 0);

		// close
		ctx->ItemClick("Close###Close");
	};

	IM_REGISTER_TEST(engine, testCategory(), "resize node popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));

		// open the resize popup via the scene graph context menu
		IM_CHECK(focusWindow(ctx, TITLE_SCENEGRAPH));
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int nodeId = sceneGraph.activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const core::String uiNodeId = core::String::format("##nodelist/%s##%i", node->name().c_str(), nodeId);
		ctx->MouseMove(uiNodeId.c_str());
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->MenuClick("//$FOCUSED/Resize");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, POPUP_TITLE_RESIZE_NODE));
		ctx->ItemInputValue("##minx", 0);
		ctx->ItemInputValue("##miny", 0);
		ctx->ItemInputValue("##minz", 0);
		ctx->ItemInputValue("##maxx", 15);
		ctx->ItemInputValue("##maxy", 15);
		ctx->ItemInputValue("##maxz", 15);
		ctx->ItemClick("###Ok");
		ctx->Yield();

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK_EQ(node->region().getWidthInVoxels(), 16);
	};

	IM_REGISTER_TEST(engine, testCategory(), "rescale node popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(newFilledScene(ctx, _sceneMgr, "rescalenodetest"));

		// open the rescale popup via the scene graph context menu
		IM_CHECK(focusWindow(ctx, TITLE_SCENEGRAPH));
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int nodeId = sceneGraph.activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const core::String uiNodeId = core::String::format("##nodelist/%s##%i", node->name().c_str(), nodeId);
		ctx->MouseMove(uiNodeId.c_str());
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->MenuClick("//$FOCUSED/Rescale content");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, POPUP_TITLE_RESCALE_NODE));
		// verify the popup opened and has the expected controls
		const ImGuiTestItemInfo voxelSize = ctx->ItemInfo("Voxel size", ImGuiTestOpFlags_NoError);
		IM_CHECK(voxelSize.ID != 0);
		const ImGuiTestItemInfo aspectRatio = ctx->ItemInfo("Maintain aspect ratio", ImGuiTestOpFlags_NoError);
		IM_CHECK(aspectRatio.ID != 0);
		ctx->ItemClick("###Ok");
		ctx->Yield(3);
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
		IM_CHECK(resetScene(ctx, _sceneMgr));
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

		IM_CHECK(resetScene(ctx, _sceneMgr));
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
		IM_CHECK(resetScene(ctx, _sceneMgr));
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
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Save");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "file load dialog")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Load");
		ctx->Yield();
		// the load dialog opens the "Select a file" popup
		IM_CHECK(focusWindow(ctx, "Select a file"));
		ctx->ItemClick("###Cancel");
	};

	IM_REGISTER_TEST(engine, testCategory(), "file save as")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Save as");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "uitest.vengi"));
		const core::String suggestedFilename = _sceneMgr->getSuggestedFilename();
		IM_CHECK(core::string::endsWith(suggestedFilename, "uitest.vengi"));
		// load the saved file and verify the name is still correct
		IM_CHECK(resetScene(ctx, _sceneMgr));
		io::FileDescription fd;
		fd.set(suggestedFilename);
		IM_CHECK(_sceneMgr->load(fd));
		for (int i = 0; i < 20 && _sceneMgr->isLoading(); ++i) {
			ctx->Yield();
		}
		IM_CHECK(!_sceneMgr->isLoading());
		IM_CHECK_STR_EQ(_sceneMgr->getSuggestedFilename().c_str(), suggestedFilename.c_str());
	};

	IM_REGISTER_TEST(engine, testCategory(), "bindings dialog delete")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Edit/Bindings");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, "Bindings"));

		// count the bindings before deletion
		const util::BindMap bindingsBefore = app()->keybindingHandler().bindings();
		const int countBefore = (int)bindingsBefore.size();
		IM_CHECK(countBefore > 0);

		// click the first delete button in the bindings table
		ctx->ItemClick("**/###del-key-0");
		ctx->Yield();

		const util::BindMap bindingsAfter = app()->keybindingHandler().bindings();
		const int countAfter = (int)bindingsAfter.size();
		IM_CHECK_EQ(countAfter, countBefore - 1);

		// close the dialog by clicking the window close button
		ctx->WindowClose("");
	};

	IM_REGISTER_TEST(engine, testCategory(), "bindings dialog change binding")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Edit/Bindings");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, "Bindings"));

		// double-click first row to start recording a new binding
		ctx->ItemDoubleClick("**/##row-0");
		ctx->Yield();

		// press a key to record (F9 is unlikely to conflict)
		ctx->KeyPress(ImGuiKey_F9);
		ctx->Yield();

		// capture the bindings before applying
		const util::BindMap bindingsBefore = app()->keybindingHandler().bindings();
		const int countBefore = (int)bindingsBefore.size();

		// apply the new binding
		ctx->ItemClick("Apply");
		ctx->Yield();

		// validate the binding: count should remain the same (one removed, one added)
		const util::BindMap bindingsAfter = app()->keybindingHandler().bindings();
		const int countAfter = (int)bindingsAfter.size();
		IM_CHECK_EQ(countAfter, countBefore);

		// verify that a binding with F9 key exists
		bool foundF9 = false;
		for (auto it = bindingsAfter.begin(); it != bindingsAfter.end(); ++it) {
			const core::String &keyBinding = util::KeyBindingHandler::toString(it->first, it->second.modifier, it->second.count);
			if (core::string::icontains(keyBinding, "f9")) {
				foundF9 = true;
				break;
			}
		}
		IM_CHECK(foundF9);

		// close the dialog by clicking the window close button
		ctx->WindowClose("");
	};

	IM_REGISTER_TEST(engine, testCategory(), "bindings dialog change keymap")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Edit/Bindings");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, "Bindings"));

		const core::VarPtr uiKeyMap = core::Var::getVar(cfg::UIKeyMap);
		const int initialKeyMap = uiKeyMap->intVal();

		// change to a different keymap via the combo
		// the keymaps are: 0=Magicavoxel, 1=Blender, 2=Vengi, 3=Qubicle, 4=Goxel, 5=3dsMax
		const int targetKeyMap = (initialKeyMap == 1) ? 2 : 1;
		const char *targetName = (targetKeyMap == 1) ? "Blender" : "Vengi";
		const core::String comboPath = core::String::format("Keymap/%s", targetName);
		ctx->ComboClick(comboPath.c_str());
		ctx->Yield();

		IM_CHECK_EQ(uiKeyMap->intVal(), targetKeyMap);

		// restore the original keymap
		const char *originalName = (initialKeyMap == 0) ? "Magicavoxel" : ((initialKeyMap == 1) ? "Blender" : ((initialKeyMap == 2) ? "Vengi" : "Qubicle"));
		const core::String restorePath = core::String::format("Keymap/%s", originalName);
		ctx->ComboClick(restorePath.c_str());
		ctx->Yield();

		IM_CHECK_EQ(uiKeyMap->intVal(), initialKeyMap);

		// close the dialog by clicking the window close button
		ctx->WindowClose("");
	};

	IM_REGISTER_TEST(engine, testCategory(), "bindings dialog 3dsmax keymap")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("Edit/Bindings");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, "Bindings"));

		const core::VarPtr uiKeyMap = core::Var::getVar(cfg::UIKeyMap);
		const int initialKeyMap = uiKeyMap->intVal();

		ctx->ComboClick("Keymap/3dsMax");
		ctx->Yield();

		const util::BindMap bindings = app()->keybindingHandler().bindings();
		bool altMiddleRotate = false;
		bool middlePan = false;
		bool rightMouseRotate = false;
		bool leftAltPan = false;
		for (auto it = bindings.begin(); it != bindings.end(); ++it) {
			const core::String &command = it->second.command;
			const core::String keyBinding = util::KeyBindingHandler::toString(it->first, it->second.modifier, it->second.count);
			if (command == "+camera_rotate" && keyBinding == "alt+middle_mouse") {
				altMiddleRotate = true;
			} else if (command == "+camera_pan" && keyBinding == "middle_mouse") {
				middlePan = true;
			} else if (command == "+camera_rotate" && keyBinding == "right_mouse") {
				rightMouseRotate = true;
			} else if (command == "+camera_pan" && keyBinding == "left_alt") {
				leftAltPan = true;
			}
		}

		IM_CHECK(altMiddleRotate);
		IM_CHECK(middlePan);
		IM_CHECK(!rightMouseRotate);
		IM_CHECK(!leftAltPan);

		const char *originalName = (initialKeyMap == 0) ? "Magicavoxel" :
			((initialKeyMap == 1) ? "Blender" :
			((initialKeyMap == 2) ? "Vengi" :
			((initialKeyMap == 3) ? "Qubicle" :
			((initialKeyMap == 4) ? "Goxel" : "3dsMax"))));
		const core::String restorePath = core::String::format("Keymap/%s", originalName);
		ctx->ComboClick(restorePath.c_str());
		ctx->Yield();

		IM_CHECK_EQ(uiKeyMap->intVal(), initialKeyMap);

		ctx->WindowClose("");
	};

	IM_REGISTER_TEST(engine, testCategory(), "model node settings popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));

		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int modelsBefore = (int)sceneGraph.size(scenegraph::SceneGraphNodeType::Model);

		// open the model node settings popup via the scene graph panel toolbar
		IM_CHECK(focusWindow(ctx, TITLE_SCENEGRAPH));
		ctx->ItemClick("toolbar/###button0");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, POPUP_TITLE_MODEL_NODE_SETTINGS));
		ctx->ItemInputValue("##modelsettingsname", "test model node");
		ctx->ItemInputValue("##posx", 1);
		ctx->ItemInputValue("##posy", 2);
		ctx->ItemInputValue("##posz", 3);
		ctx->ItemInputValue("Width", 16);
		ctx->ItemInputValue("Height", 16);
		ctx->ItemInputValue("Depth", 16);
		ctx->ItemClick("###Ok");
		ctx->Yield();

		IM_CHECK_EQ((int)sceneGraph.size(scenegraph::SceneGraphNodeType::Model), modelsBefore + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "model unreference popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));

		// create a reference node
		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		command::executeCommands("modelref");
		ctx->Yield(3);
		const int refsBefore = (int)sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		IM_CHECK(refsBefore >= 1);

		// find and activate the reference node
		int refNodeId = InvalidNodeId;
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			if ((*iter).isReferenceNode()) {
				refNodeId = (*iter).id();
				break;
			}
		}
		IM_CHECK(refNodeId != InvalidNodeId);
		_sceneMgr->nodeActivate(refNodeId);
		ctx->Yield();

		// trigger the unreference popup
		_popupModelUnreference = true;
		ctx->Yield(3);

		IM_CHECK(isPopupOpen(POPUP_TITLE_MODEL_UNREFERENCE));
		ctx->SetRef(POPUP_TITLE_MODEL_UNREFERENCE);
		ctx->ItemClick("###Yes");
		ctx->Yield(3);

		const int refsAfter = (int)sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		IM_CHECK(refsAfter < refsBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "failed to save popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		// trigger the failed to save popup
		_popupFailedToSave = true;
		ctx->Yield(3);

		// the flag should have been consumed by the popup opening
		IM_CHECK(!_popupFailedToSave);

		// try to click Ok if the popup is open
		if (isPopupOpen(POPUP_TITLE_FAILED_TO_SAVE)) {
			ctx->SetRef(POPUP_TITLE_FAILED_TO_SAVE);
			ctx->ItemClick("###Ok");
			ctx->Yield();
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "volume split suggestion popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		// trigger the volume split popup
		_popupVolumeSplit = true;
		ctx->Yield(3);

		if (isPopupOpen(POPUP_TITLE_VOLUME_SPLIT)) {
			ctx->SetRef(POPUP_TITLE_VOLUME_SPLIT);
			ctx->ItemClick("###No");
			ctx->Yield();
		}
		IM_CHECK(!_popupVolumeSplit);
	};

	IM_REGISTER_TEST(engine, testCategory(), "minecraft mapping popup")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		_popupMinecraftMapping->setVal(true);
		ctx->Yield(3);
		if (isPopupOpen(POPUP_TITLE_MINECRAFTMAPPING)) {
			ctx->SetRef(POPUP_TITLE_MINECRAFTMAPPING);
			const ImGuiTestItemInfo table = ctx->ItemInfo("##minecraftmapping", ImGuiTestOpFlags_NoError);
			IM_CHECK(table.ID != 0);
			ctx->ItemClick("Close###Close");
			ctx->Yield();
		}
		IM_CHECK(!_popupMinecraftMapping->boolVal());
	};
}

} // namespace voxedit
