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
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			const core::String btnid = core::String::format("brushes/###button%d", i);
			ctx->ItemClick(btnid.c_str());
			ctx->Yield();
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "select")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Select));

		command::executeCommands("select none");
		const scenegraph::SceneGraphNode *nodeBeforeSelect = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
		IM_CHECK(nodeBeforeSelect != nullptr);
		IM_CHECK(!nodeBeforeSelect->hasSelection());

		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK(executeViewportClickArea(ctx, _sceneMgr, viewportId, ImVec2(-100, -100)));
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

#if 0
	// https://github.com/vengi-voxel/vengi/issues/457
	IM_REGISTER_TEST(engine, testCategory(), "line brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		// TODO: not all created volumes are overridden and removed on click... check why
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Line));
	};

	IM_REGISTER_TEST(engine, testCategory(), "path brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		// TODO: load or create a volume first, path needs existing volumes
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Path));
	};

	IM_REGISTER_TEST(engine, testCategory(), "paint brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		// TODO: load or create a volume first, paint needs existing volumes
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Paint));
	};
#endif

	// TODO: copy and paste
}

} // namespace voxedit
