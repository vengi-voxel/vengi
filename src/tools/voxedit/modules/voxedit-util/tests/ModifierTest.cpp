/**
 * @file
 */

#include "../modifier/Modifier.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "core/SharedPtr.h"
#include "math/tests/TestMathHelper.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxedit {

class ModifierTest : public app::AbstractTest {
protected:
	class TrackingModifierRenderer : public IModifierRenderer {
	public:
		int updateCalls = 0;
		int renderCalls = 0;
		int waitCalls = 0;
		ModifierRendererContext lastContext;

		void update(const ModifierRendererContext &ctx) override {
			++updateCalls;
			lastContext = ctx;
		}
		void render(const video::Camera &camera, const glm::mat4 &modelMatrix) override {
			++renderCalls;
		}
		void waitForPendingExtractions() override {
			++waitCalls;
		}
	};

	void SetUp() override {
		app::AbstractTest::SetUp();
		const core::VarDef uILastDirectory(cfg::UILastDirectory, "", "", "", core::CV_NOPERSIST);
		core::Var::registerVar(uILastDirectory);
		const core::VarDef clientMouseRotationSpeed(cfg::ClientMouseRotationSpeed, 0.01f, "", "", core::CV_NONE);
		core::Var::registerVar(clientMouseRotationSpeed);
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, "", "", core::CV_NONE);
		core::Var::registerVar(clientCameraZoomSpeed);
	}

	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType,
				 BrushType brushType) {
		modifier.setBrushType(brushType);
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX); // mins for aabb
		EXPECT_TRUE(modifier.beginBrush());
		if (brushType == BrushType::Shape) {
			if (modifier.shapeBrush().singleMode()) {
				EXPECT_FALSE(modifier.shapeBrush().active())
					<< "ShapeBrush is active in single mode for modifierType " << (int)modifierType;
				return;
			}
			EXPECT_TRUE(modifier.shapeBrush().active())
				<< "ShapeBrush is not active for modifierType " << (int)modifierType;
		}
		modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX); // maxs for aabb
		modifier.executeAdditionalAction();
	}

	void select(scenegraph::SceneGraphNode &node, Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		prepare(modifier, mins, maxs, ModifierType::Paint, BrushType::Select);
		scenegraph::SceneGraph sceneGraph;
		modifier.execute(sceneGraph, node);
		modifier.endBrush();
	}

	/**
	 * @brief Trigger preview generation through the normal render flow
	 *
	 * This simulates the real update cycle: first render schedules the preview
	 * update on a dirty brush, then after time advances past the threshold,
	 * the second render triggers the actual preview generation.
	 */
	void triggerPreviewUpdate(Modifier &modifier, palette::Palette &palette) {
		video::Camera camera;
		modifier.update(1.0, &camera);
		modifier.render(camera, palette); // brush is dirty - schedules preview update
		modifier.update(1.2, &camera);    // advance time past threshold
		modifier.render(camera, palette); // triggers updateBrushVolumePreview
	}
};

TEST_F(ModifierTest, testModifierAction) {
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr, core::make_shared<IModifierRenderer>());
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place, BrushType::Shape);
	voxel::RawVolume volume({-10, 10});
	bool modifierExecuted = false;
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);
	EXPECT_TRUE(modifier.execute(sceneGraph, node,
								 [&](const voxel::Region &region, ModifierType modifierType, SceneModifiedFlags flags) {
									 modifierExecuted = true;
									 EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
								 }));
	EXPECT_TRUE(modifierExecuted);
	modifier.shutdown();
}

TEST_F(ModifierTest, testModifierSelection) {
	voxel::RawVolume volume({-10, 10});
	// Fill volume with voxels in the selection area
	for (int z = -1; z <= 1; ++z) {
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr, core::make_shared<IModifierRenderer>());
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	select(node, modifier, glm::ivec3(-1), glm::ivec3(1));

	EXPECT_TRUE(node.hasSelection()) << "Node should have selection after select()";
	// Surface voxel at (1,0,0) should be selected
	EXPECT_TRUE((volume.voxel(1, 0, 0).getFlags() & voxel::FlagOutline) != 0)
		<< "Surface voxel at (1,0,0) should be selected";
	// Interior voxel at (0,0,0) should also be selected
	EXPECT_TRUE((volume.voxel(0, 0, 0).getFlags() & voxel::FlagOutline) != 0) << "Interior voxel should be selected";
	EXPECT_FALSE((volume.voxel(2, 2, 2).getFlags() & voxel::FlagOutline) != 0)
		<< "Voxel outside selection should not be selected";

	prepare(modifier, glm::ivec3(-3), glm::ivec3(3), ModifierType::Override, BrushType::Shape);
	scenegraph::SceneGraph sceneGraph;
	int modifierExecuted = 0;
	EXPECT_TRUE(modifier.execute(sceneGraph, node,
								 [&](const voxel::Region &region, ModifierType modifierType, SceneModifiedFlags flags) {
									 ++modifierExecuted;
									 EXPECT_EQ(voxel::Region(glm::ivec3(-1), glm::ivec3(1)), region);
								 }));
	EXPECT_EQ(1, modifierExecuted);
	// Surface voxels at corners should still have selection flag
	EXPECT_TRUE((volume.voxel(-1, -1, -1).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_TRUE((volume.voxel(1, 1, 1).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_FALSE((volume.voxel(2, 2, 2).getFlags() & voxel::FlagOutline) != 0);
	EXPECT_FALSE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(-2, -2, -2).getMaterial()));
	EXPECT_TRUE(voxel::isAir(volume.voxel(2, 2, 2).getMaterial()));
	modifier.shutdown();
}

TEST_F(ModifierTest, testClamp) {
	scenegraph::SceneGraph sceneGraph;
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr, core::make_shared<IModifierRenderer>());
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	voxel::RawVolume volume(voxel::Region(0, 0, 0, 10, 20, 4));
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(&volume, false);

	TextBrush &brush = modifier.textBrush();
	brush.setInput("ABC");
	brush.setFont("font.ttf");

	modifier.setBrushType(BrushType::Text);
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setGridResolution(1);
	modifier.setCursorPosition(volume.region().getLowerCenter(), voxel::FaceNames::PositiveX); // mins for aabb

	{
		brush.setBrushClamping(false);
		voxel::Region dirtyRegion;
		EXPECT_TRUE(modifier.execute(sceneGraph, node,
									 [&dirtyRegion](const voxel::Region &region, ModifierType type,
													SceneModifiedFlags flags) { dirtyRegion = region; }));
		EXPECT_EQ(dirtyRegion.getDimensionsInVoxels(), glm::ivec3(6, 9, 1));
	}
	volume.clear();
	{
		brush.setBrushClamping(true);
		voxel::Region dirtyRegion;
		EXPECT_TRUE(modifier.execute(sceneGraph, node,
									 [&dirtyRegion](const voxel::Region &region, ModifierType type,
													SceneModifiedFlags flags) { dirtyRegion = region; }));
		EXPECT_EQ(dirtyRegion.getDimensionsInVoxels(), glm::ivec3(10, 9, 1));
	}

	brush.shutdown();
}

TEST_F(ModifierTest, testPreviewShapeAABB) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	const BrushPreview &preview = modifier.brushPreview();
	EXPECT_TRUE(preview.useSimplePreview) << "AABB shape should use simple preview";
	EXPECT_TRUE(preview.simplePreviewRegion.isValid()) << "Simple preview region should be valid";
	EXPECT_EQ(preview.simplePreviewRegion, voxel::Region(glm::ivec3(-1), glm::ivec3(1)));
	EXPECT_EQ(modifier.previewVolume(), nullptr) << "Simple preview should not create a volume";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewShapeEllipse) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	prepare(modifier, glm::ivec3(-2), glm::ivec3(2), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	const BrushPreview &preview = modifier.brushPreview();
	EXPECT_FALSE(preview.useSimplePreview) << "Ellipse shape should use full volume preview";
	ASSERT_NE(modifier.previewVolume(), nullptr) << "Ellipse preview should create a volume";

	// The preview should contain at least some non-air voxels
	int voxelCount = 0;
	const voxel::RawVolume *pv = modifier.previewVolume();
	const voxel::Region &pvRegion = pv->region();
	for (int z = pvRegion.getLowerZ(); z <= pvRegion.getUpperZ(); ++z) {
		for (int y = pvRegion.getLowerY(); y <= pvRegion.getUpperY(); ++y) {
			for (int x = pvRegion.getLowerX(); x <= pvRegion.getUpperX(); ++x) {
				if (!voxel::isAir(pv->voxel(x, y, z).getMaterial())) {
					++voxelCount;
				}
			}
		}
	}
	EXPECT_GT(voxelCount, 0) << "Preview volume should contain voxels";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewPaintNeedsExistingVoxels) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	// Fill the active volume with existing voxels so paint mode has something to work with
	const int activeNodeId = mgr.sceneGraph().activeNode();
	voxel::RawVolume *volume = mgr.volume(activeNodeId);
	ASSERT_NE(volume, nullptr);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			for (int z = -1; z <= 1; ++z) {
				volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Paint, BrushType::Paint);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	// Paint mode requires existing volume, so the preview should have the copied voxels
	ASSERT_NE(modifier.previewVolume(), nullptr) << "Paint preview should create a volume";
	EXPECT_FALSE(voxel::isAir(modifier.previewVolume()->voxel(0, 0, 0).getMaterial()))
		<< "Paint preview should contain existing voxels";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewReset) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	prepare(modifier, glm::ivec3(-2), glm::ivec3(2), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);
	ASSERT_NE(modifier.previewVolume(), nullptr);

	modifier.resetPreview();
	EXPECT_EQ(modifier.previewVolume(), nullptr) << "After reset, preview volume should be nullptr";
	EXPECT_EQ(modifier.previewMirrorVolume(), nullptr) << "After reset, mirror volume should be nullptr";
	EXPECT_FALSE(modifier.brushPreview().useSimplePreview) << "After reset, simple preview should be false";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewEraseUsesPlace) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	prepare(modifier, glm::ivec3(-2), glm::ivec3(2), ModifierType::Erase, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	// Even in erase mode the preview should create voxels (uses Place internally)
	ASSERT_NE(modifier.previewVolume(), nullptr) << "Erase preview should still create a volume";
	int voxelCount = 0;
	const voxel::RawVolume *pv = modifier.previewVolume();
	const voxel::Region &pvRegion = pv->region();
	for (int z = pvRegion.getLowerZ(); z <= pvRegion.getUpperZ(); ++z) {
		for (int y = pvRegion.getLowerY(); y <= pvRegion.getUpperY(); ++y) {
			for (int x = pvRegion.getLowerX(); x <= pvRegion.getUpperX(); ++x) {
				if (!voxel::isAir(pv->voxel(x, y, z).getMaterial())) {
					++voxelCount;
				}
			}
		}
	}
	EXPECT_GT(voxelCount, 0) << "Erase preview should still show voxels (using Place internally)";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewMirror) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	modifier.shapeBrush().setMirrorAxis(math::Axis::X, glm::ivec3(0));
	prepare(modifier, glm::ivec3(1, -1, -1), glm::ivec3(3, 1, 1), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	EXPECT_NE(modifier.previewVolume(), nullptr) << "Mirror preview should create a primary volume";
	EXPECT_NE(modifier.previewMirrorVolume(), nullptr) << "Mirror preview should create a mirror volume";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewSimpleMirror) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	// AABB shape with mirror should produce simple preview with mirror region
	modifier.shapeBrush().setMirrorAxis(math::Axis::X, glm::ivec3(0));
	prepare(modifier, glm::ivec3(1, -1, -1), glm::ivec3(3, 1, 1), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	const BrushPreview &preview = modifier.brushPreview();
	EXPECT_TRUE(preview.useSimplePreview) << "AABB shape with mirror should use simple preview";
	EXPECT_TRUE(preview.simplePreviewRegion.isValid());
	EXPECT_TRUE(preview.simpleMirrorPreviewRegion.isValid()) << "Mirror region should be valid";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testPreviewNoVolume) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	// No scene created - no active volume available

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Place, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	EXPECT_EQ(modifier.previewVolume(), nullptr);
	EXPECT_EQ(modifier.previewMirrorVolume(), nullptr);
	EXPECT_FALSE(modifier.brushPreview().useSimplePreview);
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testRenderCallsRenderer) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(0, 31));

	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setCursorPosition(glm::ivec3(5), voxel::FaceNames::PositiveX);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	video::Camera camera;
	modifier.render(camera, palette);

	EXPECT_EQ(renderer->updateCalls, 1) << "Renderer update should be called once per render";
	EXPECT_EQ(renderer->renderCalls, 1) << "Renderer render should be called once per render";
	// Check that cursor position was passed to the renderer
	EXPECT_EQ(renderer->lastContext.cursorPosition, glm::ivec3(5));
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(ModifierTest, testRenderSkippedWhenLocked) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	Modifier modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	modifier.lock();

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	video::Camera camera;
	modifier.render(camera, palette);

	EXPECT_EQ(renderer->updateCalls, 0) << "Renderer should not be called when modifier is locked";
	EXPECT_EQ(renderer->renderCalls, 0) << "Renderer should not be called when modifier is locked";

	modifier.unlock();
	modifier.shutdown();
}

} // namespace voxedit
