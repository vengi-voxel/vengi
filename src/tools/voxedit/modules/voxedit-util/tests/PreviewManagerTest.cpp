/**
 * @file
 */

#include "ModifierEx.h"
#include "../modifier/PreviewManager.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "core/SharedPtr.h"
#include "palette/Palette.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PreviewManagerTest : public app::AbstractTest {
protected:
	bool onInitApp() override {
		app::AbstractTest::onInitApp();
		_testApp->filesystem()->registerPath("brushes/");
		return _testApp->filesystem()->registerPath("selectionmodes/");
	}

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
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX);
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
		modifier.setCursorPosition(maxs, voxel::FaceNames::PositiveX);
		modifier.executeAdditionalAction();
	}

	void triggerPreviewUpdate(Modifier &modifier, palette::Palette &palette) {
		video::Camera camera;
		modifier.update(1.0, &camera);
		modifier.render(camera, palette);
		modifier.update(1.2, &camera);
		modifier.render(camera, palette);
	}
};

TEST_F(PreviewManagerTest, testPreviewShapeAABB) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
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

TEST_F(PreviewManagerTest, testPreviewShapeEllipse) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
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

TEST_F(PreviewManagerTest, testPreviewPaintNeedsExistingVoxels) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

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

	ModifierEx modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	prepare(modifier, glm::ivec3(-1), glm::ivec3(1), ModifierType::Paint, BrushType::Paint);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

	ASSERT_NE(modifier.previewVolume(), nullptr) << "Paint preview should create a volume";
	EXPECT_FALSE(voxel::isAir(modifier.previewVolume()->voxel(0, 0, 0).getMaterial()))
		<< "Paint preview should contain existing voxels";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(PreviewManagerTest, testPreviewReset) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
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

TEST_F(PreviewManagerTest, testPreviewEraseUsesPlace) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	prepare(modifier, glm::ivec3(-2), glm::ivec3(2), ModifierType::Erase, BrushType::Shape);

	palette::Palette palette;
	palette.tryAdd(color::RGBA{255, 0, 0, 255});

	triggerPreviewUpdate(modifier, palette);

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

TEST_F(PreviewManagerTest, testPreviewMirror) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
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

TEST_F(PreviewManagerTest, testPreviewSimpleMirror) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));

	ModifierEx modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

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

TEST_F(PreviewManagerTest, testPreviewNoVolume) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());

	ModifierEx modifier(&mgr, renderer);
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

} // namespace voxedit
