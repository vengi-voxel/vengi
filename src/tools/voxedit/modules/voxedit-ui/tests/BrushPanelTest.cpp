/**
 * @file
 */

#include "../BrushPanel.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

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

static bool setModifierType(ImGuiTestContext *ctx, voxedit::Modifier &modifier, ModifierType type) {
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
	voxedit::Modifier &modifier = sceneMgr->modifier();

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

		const bool normalPaletteMode = viewModeNormalPalette(_viewMode->intVal());

		// now we can focus the brush panel
		IM_CHECK(focusWindow(ctx, id));

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		int buttonIdx = 0;
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			if (i == (int)BrushType::Normal && !normalPaletteMode) {
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

	IM_REGISTER_TEST(engine, testCategory(), "select color actions")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Select));

		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(1, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(2, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 2)));

		auto selectedCount = [&]() {
			int count = 0;
			voxelutil::visitVolume(*node->volume(), [&count](int x, int y, int z, const voxel::Voxel &v) {
				if ((v.getFlags() & voxel::FlagOutline) != 0u) {
					++count;
				}
			});
			return count;
		};

		command::executeCommands("select all");
		ctx->Yield(3);
		IM_CHECK(node->hasSelection());
		IM_CHECK_EQ(selectedCount(), 3);

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("Select Only Color");
		ctx->Yield(3);
		IM_CHECK_EQ(selectedCount(), 2);
		voxel::RawVolume *v = node->volume();
		IM_CHECK((v->voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0u);
		IM_CHECK((v->voxel(1, 0, 0).getFlags() & voxel::FlagOutline) != 0u);
		IM_CHECK((v->voxel(2, 0, 0).getFlags() & voxel::FlagOutline) == 0u);

		ctx->ItemClick("Deselect Color");
		ctx->Yield(3);
		IM_CHECK_EQ(selectedCount(), 0);

		command::executeCommands("select all");
		ctx->Yield(3);
		IM_CHECK_EQ(selectedCount(), 3);

		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 2));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("Color Selected");
		ctx->Yield(3);
		IM_CHECK_EQ(v->voxel(0, 0, 0).getColor(), 2);
		IM_CHECK_EQ(v->voxel(1, 0, 0).getColor(), 2);
		IM_CHECK_EQ(v->voxel(2, 0, 0).getColor(), 2);
	};

	IM_REGISTER_TEST(engine, testCategory(), "shape brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Shape));
	};

	IM_REGISTER_TEST(engine, testCategory(), "plane brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(runBrushModifiers(this, ctx, id, _sceneMgr, BrushType::Plane));
	};

	IM_REGISTER_TEST(engine, testCategory(), "paint brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Paint));
		// fill to create existing voxels that paint brush can operate on
		command::executeCommands("fill");
		ctx->Yield(3);
		voxedit::Modifier &modifier = _sceneMgr->modifier();
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 2));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportEditMode(ctx, _app), ImVec2(0, -50)));
		executeViewportClick();
	};

	IM_REGISTER_TEST(engine, testCategory(), "stamp brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Stamp));
		// use selection as stamp source
		command::executeCommands("fill");
		ctx->Yield(3);
		command::executeCommands("select all");
		ctx->Yield(3);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("Use selection");
		ctx->Yield(3);

		// create a new empty model node to stamp into
		const int newNodeId = _sceneMgr->addModelChild("stamp target", 32, 32, 32);
		IM_CHECK(newNodeId != InvalidNodeId);
		ctx->Yield(3);

		// verify the new node is empty
		scenegraph::SceneGraphNode *newModel = _sceneMgr->sceneGraphModelNode(newNodeId);
		IM_CHECK(newModel != nullptr);
		IM_CHECK(voxelutil::countVoxels(*newModel->volume()) == 0);

		// center on viewport and place the stamp
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportEditMode(ctx, _app), ImVec2(0, -50)));
		executeViewportClick();

		// validate that voxels were placed by the stamp
		IM_CHECK(voxelutil::countVoxels(*newModel->volume()) > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "normal brush")->TestFunc = [=](ImGuiTestContext *ctx) {
		// switch to "All" view mode so the Normal brush button is visible
		IM_CHECK(changeViewMode(ctx, ViewMode::All));
		ctx->Yield(3);

		// activeBrush creates a new scene internally, so fill afterwards
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Normal));

		// fill to create existing voxels that normal brush can operate on
		command::executeCommands("fill");
		ctx->Yield(3);

		// verify all voxels have NO_NORMAL after filling
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(node != nullptr);
		bool allNoNormal = true;
		voxelutil::visitVolume(*node->volume(), [&](int x, int y, int z, const voxel::Voxel &v) {
			if (v.getNormal() != NO_NORMAL) {
				allNoNormal = false;
			}
		});
		IM_CHECK(allNoNormal);

		voxedit::Modifier &modifier = _sceneMgr->modifier();

		// set a specific normal index on the modifier (the value the normal brush will paint)
		const uint8_t expectedNormal = 7;
		modifier.setNormalColorIndex(expectedNormal);

		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportEditMode(ctx, _app), ImVec2(0, -50)));
		executeViewportClick();

		// find any voxel that had its normal changed to the expected value
		node = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(node != nullptr);
		bool foundPaintedNormal = false;
		voxelutil::visitVolume(*node->volume(), [&](int x, int y, int z, const voxel::Voxel &v) {
			if (v.getNormal() == expectedNormal) {
				foundPaintedNormal = true;
			}
		});
		IM_CHECK(foundPaintedNormal);

		// reset view mode back to default
		IM_CHECK(changeViewMode(ctx, ViewMode::Default));
	};

	IM_REGISTER_TEST(engine, testCategory(), "shape brush type combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Shape));

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		ShapeBrush &brush = modifier.shapeBrush();

		for (int i = 0; i < (int)ShapeType::Max; ++i) {
			const core::String label = core::String::format("Shape/%s %s", ShapeTypeIcons[i], ShapeTypeStr[i]);
			ctx->ComboClick(label.c_str());
			ctx->Yield();
			IM_CHECK_EQ((int)brush.shapeType(), i);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "paint brush mode combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Paint));
		command::executeCommands("fill");
		ctx->Yield(3);

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		PaintBrush &brush = modifier.paintBrush();

		ctx->ComboClick("Mode/Brighten");
		ctx->Yield();
		IM_CHECK_EQ((int)brush.paintMode(), (int)PaintBrush::PaintMode::Brighten);

		ctx->ComboClick("Mode/Darken");
		ctx->Yield();
		IM_CHECK_EQ((int)brush.paintMode(), (int)PaintBrush::PaintMode::Darken);
	};

	IM_REGISTER_TEST(engine, testCategory(), "line brush controls")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Line));

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		LineBrush &brush = modifier.lineBrush();

		const bool contBefore = brush.continuous();
		ctx->ItemClick("Continuous");
		ctx->Yield();
		IM_CHECK(brush.continuous() != contBefore);

		const bool bezBefore = brush.bezier();
		ctx->ItemClick("Bezier");
		ctx->Yield();
		IM_CHECK(brush.bezier() != bezBefore);

		ctx->ItemInputValue("Thickness", 3);
		ctx->Yield();
		IM_CHECK_EQ(brush.thickness(), 3);
	};

	IM_REGISTER_TEST(engine, testCategory(), "text brush controls")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Text));

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		TextBrush &brush = modifier.textBrush();

		ctx->ItemInputValue("Text", "Hello");
		ctx->Yield();
		IM_CHECK(brush.input() == "Hello");

		ctx->ItemInputValue(ICON_LC_MOVE_VERTICAL, 24);
		ctx->Yield();
		IM_CHECK_EQ(brush.size(), 24);
	};

	IM_REGISTER_TEST(engine, testCategory(), "sculpt brush mode combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Sculpt));

		// sculpt brush needs a selection to show controls
		command::executeCommands("fill");
		ctx->Yield(3);
		command::executeCommands("select all");
		ctx->Yield(3);

		IM_CHECK(focusWindow(ctx, id));
		voxedit::Modifier &modifier = _sceneMgr->modifier();
		SculptBrush &brush = modifier.sculptBrush();

		for (int i = 0; i < (int)SculptMode::Max; ++i) {
			const core::String label = core::String::format("Sculpt mode/%s %s", SculptModeIcons[i], _(SculptModeStr[i]));
			ctx->ComboClick(label.c_str());
			ctx->Yield();
			IM_CHECK_EQ((int)brush.sculptMode(), i);
		}

		command::executeCommands("select none");
	};

	IM_REGISTER_TEST(engine, testCategory(), "select brush mode combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Select));

		IM_CHECK(focusWindow(ctx, id));
		voxedit::Modifier &modifier = _sceneMgr->modifier();
		SelectBrush &brush = modifier.selectBrush();

		for (int i = 0; i < (int)SelectMode::Max; ++i) {
			if ((SelectMode)i == SelectMode::Script) {
				continue;
			}
			const core::String label = core::String::format("Select mode/%s %s", SelectModeIcons[i], _(SelectModeStr[i]));
			ctx->ComboClick(label.c_str());
			ctx->Yield();
			IM_CHECK_EQ((int)brush.selectMode(), i);
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "ruler brush controls")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Ruler));
		IM_CHECK(focusWindow(ctx, id));

		voxedit::Modifier &modifier = _sceneMgr->modifier();
		RulerBrush &brush = modifier.rulerBrush();

		const bool before = brush.useReferencePos();
		ctx->ItemClick("Use reference position");
		ctx->Yield();
		IM_CHECK(brush.useReferencePos() != before);
		ctx->ItemClick("Use reference position");
		ctx->Yield();
		IM_CHECK(brush.useReferencePos() == before);
	};

	IM_REGISTER_TEST(engine, testCategory(), "script brush rescan")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Script));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("Rescan###Rescan");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "transform brush mode combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activeBrush(this, ctx, id, _sceneMgr, BrushType::Transform));
		// transform brush needs a selection
		command::executeCommands("fill");
		ctx->Yield(3);
		command::executeCommands("select all");
		ctx->Yield(3);

		IM_CHECK(focusWindow(ctx, id));
		voxedit::Modifier &modifier = _sceneMgr->modifier();
		TransformBrush &brush = modifier.transformBrush();

		for (int i = 0; i < (int)TransformMode::Max; ++i) {
			const core::String label = core::String::format("Transform mode/%s %s", TransformModeIcons[i], _(TransformModeStr[i]));
			ctx->ComboClick(label.c_str());
			ctx->Yield();
			IM_CHECK_EQ((int)brush.transformMode(), i);
		}

		command::executeCommands("select none");
	};
}

} // namespace voxedit
