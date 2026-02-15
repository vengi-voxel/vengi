/**
 * @file
 */

#include "../BrushPanel.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"
#include "voxel/Voxel.h"

namespace voxedit {

static bool activeBrush(BrushPanel *panel, ImGuiTestContext *ctx, const char *id, const SceneManagerPtr &sceneMgr, BrushType type) {
	IM_CHECK_RETV(sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)), false);

	IM_CHECK_SILENT_RETV(activateViewportEditMode(ctx, panel->app()),	 false);

	// now we can focus the brush panel
	IM_CHECK_SILENT_RETV(panel->focusWindow(ctx, id), false);

	const core::String buttonId = core::String::format("brushes/###button%d", (int)type);
	ctx->ItemClick(buttonId.c_str());
	ctx->Yield();

	IM_CHECK_RETV(sceneMgr->modifier().brushType() == type, false);

	return true;
}

static bool setModifierType(ImGuiTestContext *ctx, voxedit::ModifierFacade &modifier, ModifierType type) {
	if (type == ModifierType::ColorPicker) {
		IM_CHECK_RETV(modifier.brushType() == BrushType::None, false);
		ctx->ItemClick("modifiers/###button1");
	} else {
		Brush *brush = modifier.currentBrush();
		IM_CHECK_RETV(brush != nullptr, false);
		IM_CHECK_RETV(brush->modifierType(type) == type, false);
		if (type == ModifierType::Place) {
			ctx->ItemClick("modifiers/###button0");
		} else if (type == ModifierType::Erase) {
			ctx->ItemClick("modifiers/###button1");
		} else if (type == ModifierType::Override) {
			ctx->ItemClick("modifiers/###button2");
		} else {
			IM_ERRORF("Unsupported modifier type given: %i", (int)type);
			return false;
		}
	}
	IM_CHECK_RETV(modifier.modifierType() == type, false);
	ctx->Yield();
	return true;
}

static bool runBrushModifiers(BrushPanel *panel, ImGuiTestContext *ctx, const char *id, const SceneManagerPtr &sceneMgr, BrushType type) {
	IM_CHECK_RETV(activeBrush(panel, ctx, id, sceneMgr, type), false);
	voxedit::ModifierFacade &modifier = sceneMgr->modifier();

	setModifierType(ctx, modifier, ModifierType::Place);
	IM_CHECK_RETV(centerOnViewport(ctx, sceneMgr, viewportEditMode(ctx, panel->app()), ImVec2(0, -50)), false);
	executeViewportClick();

	// change the cursor voxel
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	setModifierType(ctx, modifier, ModifierType::Override);
	IM_CHECK_RETV(centerOnViewport(ctx, sceneMgr, viewportEditMode(ctx, panel->app()), ImVec2(0, -50)), false);
	executeViewportClick();

	setModifierType(ctx, modifier, ModifierType::Erase);
	IM_CHECK_RETV(centerOnViewport(ctx, sceneMgr, viewportEditMode(ctx, panel->app()), ImVec2(0, -50)), false);
	executeViewportClick();

	// place is one voxel, override just changed the color of the voxel and erase will wipe it completely again
	IM_CHECK_RETV(voxelCount(sceneMgr) == 0, false);

	return true;
}

void BrushPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "cycle brush types")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));

		// now we can focus the brush panel
		IM_CHECK(focusWindow(ctx, id));

		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		int buttonIdx = 0;
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			if (i == (int)BrushType::Normal) {
				continue;
			}
			const core::String btnid = core::String::format("brushes/###button%d", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			ctx->Yield();
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
			++buttonIdx;
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "select")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Select));

		// fill a flat layer of voxels so the camera ray will reliably hit something
		command::executeCommands("fill");
		ctx->Yield(3);

		command::executeCommands("select none");
		const scenegraph::SceneGraphNode *nodeBeforeSelect = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
		IM_CHECK(nodeBeforeSelect != nullptr);
		IM_CHECK(!nodeBeforeSelect->hasSelection());

		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK(executeViewportClickArea(ctx, _sceneMgr, viewportId, ImVec2(-100, -100)));
		// The AABB brush requires a second action to complete the 3D selection when the
		// drag only spans 2 dimensions (needsAdditionalAction returns true)
		executeViewportClick();
		const scenegraph::SceneGraphNode *nodeAfterSelect = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
		IM_CHECK(nodeAfterSelect != nullptr);
		IM_CHECK(nodeAfterSelect->hasSelection());

		command::executeCommands("select none");
	};

	IM_REGISTER_TEST(engine, testCategory(), "shape brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Shape));
	};

	IM_REGISTER_TEST(engine, testCategory(), "plane brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Plane));
	};

	IM_REGISTER_TEST(engine, testCategory(), "paint brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		// fill to create existing voxels that paint brush can operate on
		command::executeCommands("fill");
		ctx->Yield(3);
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Paint));
		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 2));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportEditMode(ctx, _app), ImVec2(0, -50)));
		executeViewportClick();
	};

	IM_REGISTER_TEST(engine, testCategory(), "stamp brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Stamp));
		// use selection as stamp source
		command::executeCommands("fill");
		ctx->Yield(3);
		command::executeCommands("select all");
		ctx->Yield(3);
		command::executeCommands("stampbrushuseselection");
		ctx->Yield(3);
	};

	IM_REGISTER_TEST(engine, testCategory(), "copy and paste")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		// fill and select to have something to copy
		command::executeCommands("fill");
		ctx->Yield(3);
		command::executeCommands("select all");
		ctx->Yield(3);
		command::executeCommands("copy");
		ctx->Yield(3);
		command::executeCommands("paste");
		ctx->Yield(3);
	};
}

} // namespace voxedit
