/**
 * @file
 */

#include "ModifierEx.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "../modifier/PreviewManager.h"
#include "app/tests/AbstractTest.h"
#include "command/Command.h"
#include "core/SharedPtr.h"
#include "palette/Palette.h"
#include "palette/NormalPalette.h"
#include "memento/MementoHandler.h"
#include "voxel/VoxelNormalUtil.h"
#include "voxelrender/RenderContext.h"
#include "video/Camera.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
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
		int clearCalls = 0;
		ModifierRendererContext lastContext;

		void update(const ModifierRendererContext &ctx) override {
			++updateCalls;
			lastContext = ctx;
		}
		void render(voxelrender::RenderContext &renderContext, const video::Camera &camera, const glm::mat4 &modelMatrix) override {
			++renderCalls;
		}
		void waitForPendingExtractions() override {
			++waitCalls;
		}
		void clearBrushVolumes() override {
			++clearCalls;
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
		const core::VarDef renderNormals(cfg::RenderNormals, true, "", "", core::CV_NOPERSIST);
		core::Var::registerVar(renderNormals);
	}

	void prepare(Modifier &modifier, const glm::ivec3 &mins, const glm::ivec3 &maxs, ModifierType modifierType,
				 BrushType brushType) {
		if (brushType == BrushType::Shape) {
			modifier.shapeBrush().setBoxMode();
		}
		if (brushType == BrushType::Paint) {
			modifier.paintBrush().setBoxMode();
		}
		modifier.setBrushType(brushType);
		modifier.setModifierType(modifierType);
		modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
		modifier.setGridResolution(1);
		modifier.setCursorPosition(mins, voxel::FaceNames::PositiveX);
		EXPECT_TRUE(modifier.beginBrush());
		if (brushType == BrushType::Shape) {
			if (modifier.shapeBrush().strokeMode()) {
				EXPECT_FALSE(modifier.shapeBrush().active())
					<< "ShapeBrush is active in stroke mode for modifierType " << (int)modifierType;
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
		voxelrender::RenderContext renderContext;
		modifier.update(1.0, &camera);
		modifier.render(renderContext, camera, palette);
		modifier.update(1.2, &camera);
		modifier.render(renderContext, camera, palette);
	}
};

TEST_F(PreviewManagerTest, testPlanePreviewTracksExtrusionHeightUntilRelease) {
	for (ModifierType mode : {ModifierType::Place, ModifierType::Erase}) {
		SCOPED_TRACE((int)mode);
		core::SharedPtr<TrackingModifierRenderer> renderer = core::make_shared<TrackingModifierRenderer>();
		SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
						 core::make_shared<ISceneRenderer>(), renderer);
		mgr.construct();
		ASSERT_TRUE(mgr.init());
		ASSERT_TRUE(mgr.newScene(true, "plane preview", voxel::Region(-2, 5)));
		scenegraph::SceneGraphNode &node = mgr.sceneGraph().node(mgr.activeNode());
		voxel::RawVolume *volume = node.volume();
		const voxel::Voxel ground(voxel::VoxelType::Generic, 1);
		for (int x = 0; x < 3; ++x) {
			for (int z = 0; z < 3; ++z) {
				volume->setVoxel(x, -1, z, ground);
				volume->setVoxel(x, 0, z, ground);
				volume->setVoxel(x, 1, z, ground);
			}
		}
		mgr.modified(node.uuid(), voxel::Region(0, -1, 0, 2, 1, 2));
		const voxel::RawVolume original(*volume);
		const core::UUID uuid = node.uuid();
		const size_t states = mgr.mementoHandler().stateSize();
		Modifier &modifier = mgr.modifier();
		modifier.setBrushType(BrushType::Plane);
		modifier.setModifierType(mode);
		modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 2));
		modifier.setHitCursorVoxel(ground);
		const int initial = mode == ModifierType::Place ? 2 : 1;
		const int direction = mode == ModifierType::Place ? 1 : -1;
		modifier.setCursorPosition(glm::ivec3(1, initial, 1), voxel::FaceNames::PositiveY);
		ASSERT_TRUE(modifier.beginEdit());
		const int heights[] = {initial, initial + direction * 2, initial + direction};
		for (int height : heights) {
			SCOPED_TRACE(height);
			modifier.setCursorPosition(glm::ivec3(1, height, 1), voxel::FaceNames::PositiveY);
			modifier.previewManager().updateBrushVolumePreview(modifier, node.palette(), volume, mgr.sceneGraph());
			const voxel::RawVolume *preview = modifier.previewManager().previewVolume();
			ASSERT_NE(nullptr, preview);
			EXPECT_EQ(voxel::Region(0, glm::min(initial, height), 0, 2, glm::max(initial, height), 2), preview->region());
			EXPECT_FALSE(modifier.previewManager().brushPreview().useSimplePreview);
			for (int x = 0; x < 3; ++x) {
				for (int z = 0; z < 3; ++z) {
					for (int y = glm::min(initial, height); y <= glm::max(initial, height); ++y) {
						EXPECT_EQ(mode == ModifierType::Place ? 2 : 1, preview->voxel(x, y, z).getColor());
						EXPECT_FALSE(voxel::isAir(preview->voxel(x, y, z).getMaterial()));
					}
				}
			}
			core::DynamicArray<BrushGizmoState> handles;
			modifier.brushHandles(handles);
			EXPECT_TRUE(handles.empty());
			EXPECT_EQ(0, core_memcmp(original.data(), volume->data(), voxel::RawVolume::size(volume->region())));
			EXPECT_EQ(states, mgr.mementoHandler().stateSize());
		}
		modifier.finishEdit();
		EXPECT_FALSE(modifier.isEditActive());
		if (mode == ModifierType::Place) {
			EXPECT_EQ(2, volume->voxel(0, 3, 0).getColor());
			EXPECT_TRUE(voxel::isAir(volume->voxel(0, 4, 0).getMaterial()));
		} else {
			EXPECT_TRUE(voxel::isAir(volume->voxel(0, 0, 0).getMaterial()));
			EXPECT_EQ(ground, volume->voxel(0, -1, 0));
		}
		ASSERT_TRUE(mgr.undo());
		volume = mgr.sceneGraphNodeByUUID(uuid)->volume();
		EXPECT_EQ(0, core_memcmp(original.data(), volume->data(), voxel::RawVolume::size(volume->region())));
		mgr.shutdown();
	}
}
TEST_F(PreviewManagerTest, testNormalPreview) {
	for (NormalBrush::PaintMode mode : {NormalBrush::PaintMode::Manual, NormalBrush::PaintMode::Auto}) {
		auto renderer = core::make_shared<TrackingModifierRenderer>();
		SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
						 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
		mgr.construct();
		ASSERT_TRUE(mgr.init());
		ASSERT_TRUE(mgr.newScene(true, "normals", voxel::Region(-1, 1)));
		scenegraph::SceneGraphNode &node = mgr.sceneGraph().node(mgr.activeNode());
		palette::NormalPalette normals;
		normals.tiberianSun();
		node.setNormalPalette(normals);
		const voxel::Voxel original(voxel::VoxelType::Generic, 3, 42);
		for (int z = -1; z <= 1; ++z) {
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) {
					node.volume()->setVoxel(x, y, z, original);
				}
			}
		}
		ModifierEx modifier(&mgr, renderer);
		modifier.construct();
		ASSERT_TRUE(modifier.init());
		modifier.normalBrush().setStrokeMode();
		modifier.normalBrush().setPaintMode(mode);
		modifier.setNormalColorIndex(7);
		const glm::ivec3 pos(1, 0, 0);
		prepare(modifier, pos, pos, ModifierType::Place, BrushType::Normal);
		uint8_t expected = 7 + NORMAL_PALETTE_OFFSET;
		if (mode == NormalBrush::PaintMode::Auto) {
			voxel::RawVolume::Sampler sampler(node.volume());
			sampler.setPosition(pos);
			expected = normals.getClosestMatch(voxel::calculateNormal(sampler, voxel::Connectivity::TwentySixConnected)) + NORMAL_PALETTE_OFFSET;
		}
		const bool canUndo = mgr.mementoHandler().canUndo();
		triggerPreviewUpdate(modifier, node.palette());
		ASSERT_NE(nullptr, modifier.previewVolume());
		EXPECT_EQ(expected, modifier.previewVolume()->voxel(pos).getNormal());
		EXPECT_EQ(original.getColor(), modifier.previewVolume()->voxel(pos).getColor());
		ASSERT_NE(nullptr, renderer->lastContext.normalPalette);
		EXPECT_NE(&node.normalPalette(), renderer->lastContext.normalPalette);
		EXPECT_EQ(node.normalPalette().hash(), renderer->lastContext.normalPalette->hash());
		EXPECT_EQ(original, node.volume()->voxel(pos));
		EXPECT_EQ(original, node.volume()->voxel(0, 0, 0));
		EXPECT_EQ(canUndo, mgr.mementoHandler().canUndo());
		if (mode == NormalBrush::PaintMode::Manual) {
			modifier.setNormalColorIndex(0);
			triggerPreviewUpdate(modifier, node.palette());
			ASSERT_NE(nullptr, modifier.previewVolume());
			EXPECT_EQ(NORMAL_PALETTE_OFFSET, modifier.previewVolume()->voxel(pos).getNormal());
			expected = NORMAL_PALETTE_OFFSET;
		}
		ASSERT_TRUE(modifier.execute(mgr.sceneGraph(), node));
		EXPECT_EQ(expected, node.volume()->voxel(pos).getNormal());
		modifier.endBrush();
		modifier.setModifierType(ModifierType::Erase);
		triggerPreviewUpdate(modifier, node.palette());
		ASSERT_NE(nullptr, modifier.previewVolume());
		voxel::Voxel cleared = original;
		cleared.setNormal(NO_NORMAL);
		EXPECT_EQ(cleared, modifier.previewVolume()->voxel(pos));
		EXPECT_EQ(expected, node.volume()->voxel(pos).getNormal());
		ASSERT_TRUE(modifier.execute(mgr.sceneGraph(), node));
		EXPECT_EQ(cleared, node.volume()->voxel(pos));
		EXPECT_EQ(original, node.volume()->voxel(0, 0, 0));
		modifier.endBrush();
		modifier.shutdown();
		mgr.shutdown();
	}
}

TEST_F(PreviewManagerTest, testPaintPreviewRendersWithItsOwnPalette) {
	for (PaintBrush::PaintMode mode : {PaintBrush::PaintMode::Blend, PaintBrush::PaintMode::Blur}) {
		for (bool mirror : {false, true}) {
			SCOPED_TRACE((int)mode);
			SCOPED_TRACE(mirror);
			auto renderer = core::make_shared<TrackingModifierRenderer>();
			SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
							 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
			mgr.construct();
			ASSERT_TRUE(mgr.init());
			ASSERT_TRUE(mgr.newScene(true, "paint preview palette", voxel::Region(-5, 5)));
			auto &node = mgr.sceneGraph().node(mgr.activeNode());
			node.palette().setColor(0, color::RGBA(40, 40, 40));
			node.palette().setColor(1, color::RGBA(200, 200, 200));
			node.palette().setSize(2);
			ModifierEx modifier(&mgr, renderer);
			modifier.construct();
			ASSERT_TRUE(modifier.init());
			modifier.setBrushType(BrushType::Paint);
			modifier.setModifierType(ModifierType::Paint);
			PaintBrush &brush = modifier.paintBrush();
			brush.setPaintMode(mode);
			brush.setRadius(0);
			brush.setBlendOpacity(0.5f);
			if (mirror) {
				brush.setMirrorAxis(math::Axis::X, glm::ivec3(0));
			}
			const glm::ivec3 pos(2, 0, 0);
			glm::ivec3 mirrorPos = pos;
			glm::ivec3 mirrorMax = pos;
			brush.getMirrorBox(mirrorPos, mirrorMax);
			for (const glm::ivec3 &center : {pos, mirrorPos}) {
				node.volume()->setVoxel(center, voxel::Voxel(voxel::VoxelType::Generic, 0));
				node.volume()->setVoxel(center + glm::ivec3(1, 0, 0), voxel::Voxel(voxel::VoxelType::Generic, 1));
			}
			const voxel::RawVolume original(*node.volume());
			const uint64_t paletteHash = node.palette().hash();
			const size_t history = mgr.mementoHandler().stateSize();
			modifier.setCursorPosition(pos, voxel::FaceNames::PositiveY);
			modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 1));
			modifier.previewManager().updateBrushVolumePreview(modifier, node.palette(), node.volume(), mgr.sceneGraph());
			brush.markClean();
			video::Camera camera;
			voxelrender::RenderContext renderContext;
			modifier.render(renderContext, camera, node.palette());
			const ModifierRendererContext &rendered = renderer->lastContext;
			ASSERT_NE(nullptr, rendered.previewVolume);
			ASSERT_NE(nullptr, rendered.palette);
			EXPECT_NE(&node.palette(), rendered.palette);
			scenegraph::SceneGraphNode expected(scenegraph::SceneGraphNodeType::Model);
			expected.setVolume(new voxel::RawVolume(original));
			expected.setPalette(node.palette());
			ModifierVolumeWrapper output(expected, ModifierType::Paint);
			BrushContext input = modifier.brushContext();
			input.preview = true;
			ASSERT_TRUE(brush.execute(mgr.sceneGraph(), output, input));
			const auto expectedColor = [&](const glm::ivec3 &p) {
				return expected.palette().color(expected.volume()->voxel(p).getColor());
			};
			EXPECT_NE(node.palette().color(0), expectedColor(pos));
			EXPECT_EQ(expectedColor(pos), rendered.palette->color(rendered.previewVolume->voxel(pos).getColor()));
			if (mirror) {
				ASSERT_NE(nullptr, rendered.previewMirrorVolume);
				EXPECT_EQ(expectedColor(mirrorPos),
						  rendered.palette->color(rendered.previewMirrorVolume->voxel(mirrorPos).getColor()));
			}
			EXPECT_EQ(paletteHash, node.palette().hash());
			EXPECT_EQ(history, mgr.mementoHandler().stateSize());
			EXPECT_EQ(0, core_memcmp(original.data(), node.volume()->data(), voxel::RawVolume::size(original.region())));
			modifier.shutdown();
			mgr.shutdown();
		}
	}
}

TEST_F(PreviewManagerTest, testBlurPreviewMatchesFullVolumeIncludingMirrors) {
	for (bool mirror : {false, true}) {
		auto renderer = core::make_shared<TrackingModifierRenderer>();
		SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
						 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
		mgr.construct();
		ASSERT_TRUE(mgr.init());
		ASSERT_TRUE(mgr.newScene(true, "blur preview", voxel::Region(-5, 5)));
		auto &node = mgr.sceneGraph().node(mgr.activeNode());
		palette::Palette palette;
		for (int i = 0; i < 256; ++i) {
			palette.setColor(i, color::RGBA(i, i, i));
		}
		node.setPalette(palette);
		ModifierEx modifier(&mgr, renderer);
		modifier.construct();
		ASSERT_TRUE(modifier.init());
		modifier.setBrushType(BrushType::Paint);
		modifier.setModifierType(ModifierType::Paint);
		PaintBrush &brush = modifier.paintBrush();
		brush.setPaintMode(PaintBrush::PaintMode::Blur);
		brush.setRadius(0);
		brush.setBlendOpacity(1.0f);
		if (mirror) {
			brush.setMirrorAxis(math::Axis::X, glm::ivec3(0));
		}
		const glm::ivec3 pos(2, 0, 0);
		glm::ivec3 mirrorPos = pos;
		glm::ivec3 mirrorMax = pos;
		brush.getMirrorBox(mirrorPos, mirrorMax);
		for (const glm::ivec3 &center : {pos, mirrorPos}) {
			node.volume()->setVoxel(center, voxel::Voxel(voxel::VoxelType::Generic, 0, 5));
			node.volume()->setVoxel(center + glm::ivec3(1, 0, 0), voxel::Voxel(voxel::VoxelType::Generic, 200, 5));
		}
		const voxel::RawVolume original(*node.volume());
		const uint64_t paletteHash = node.palette().hash();
		const size_t history = mgr.mementoHandler().stateSize();
		modifier.setCursorPosition(pos, voxel::FaceNames::PositiveY);
		modifier.setCursorVoxel(voxel::Voxel(voxel::VoxelType::Generic, 1));
		PreviewManager preview;
		preview.construct();
		ASSERT_TRUE(preview.init(renderer));
		preview.updateBrushVolumePreview(modifier, node.palette(), node.volume(), mgr.sceneGraph());
		scenegraph::SceneGraphNode expected(scenegraph::SceneGraphNodeType::Model);
		expected.setVolume(new voxel::RawVolume(original));
		expected.setPalette(node.palette());
		ModifierVolumeWrapper expectedOutput(expected, ModifierType::Paint);
		BrushContext input = modifier.brushContext();
		input.preview = true;
		ASSERT_TRUE(brush.execute(mgr.sceneGraph(), expectedOutput, input));
		ASSERT_NE(nullptr, preview.previewVolume());
		EXPECT_EQ(voxel::Region(pos, pos), preview.previewVolume()->region());
		EXPECT_FALSE(original.voxel(pos) == expected.volume()->voxel(pos));
		EXPECT_EQ(expected.volume()->voxel(pos), preview.previewVolume()->voxel(pos));
		if (mirror) {
			ASSERT_NE(nullptr, preview.previewMirrorVolume());
			EXPECT_EQ(voxel::Region(mirrorPos, mirrorPos), preview.previewMirrorVolume()->region());
			EXPECT_EQ(expected.volume()->voxel(mirrorPos), preview.previewMirrorVolume()->voxel(mirrorPos));
		}
		EXPECT_EQ(paletteHash, node.palette().hash());
		EXPECT_EQ(history, mgr.mementoHandler().stateSize());
		EXPECT_EQ(0, core_memcmp(original.data(), node.volume()->data(), voxel::RawVolume::size(original.region())));
		preview.shutdown();
		modifier.shutdown();
		mgr.shutdown();
	}
}

TEST_F(PreviewManagerTest, testZeroRadiusStrokeCursorMoves) {
	auto renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));
	ModifierEx modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());
	modifier.setBrushType(BrushType::Shape);
	modifier.shapeBrush().setStrokeMode();
	modifier.shapeBrush().setRadius(0);
	modifier.setModifierType(ModifierType::Place);
	modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	modifier.setGridResolution(1);
	palette::Palette palette;
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);
	for (int i = 0; i < 3; ++i) {
		const glm::ivec3 pos(i, 0, 0);
		modifier.setCursorPosition(pos, voxel::FaceNames::PositiveX);
		triggerPreviewUpdate(modifier, palette);
		const BrushPreview &preview = modifier.brushPreview();
		EXPECT_FALSE(preview.useSimplePreview);
		EXPECT_FALSE(preview.simplePreviewRegion.isValid());
		EXPECT_EQ(modifier.previewVolume(), nullptr);
		EXPECT_FALSE(preview.showOutlinePreview);
	}

	{
		util::ScopedVarChange scoped(cfg::VoxEditShapeFaceCursor, "false");
		for (int i = 0; i < 3; ++i) {
			const glm::ivec3 pos(i, 0, 0);
			modifier.setCursorPosition(pos, voxel::FaceNames::PositiveX);
			triggerPreviewUpdate(modifier, palette);
			const BrushPreview &preview = modifier.brushPreview();
			EXPECT_TRUE(preview.useSimplePreview);
			EXPECT_EQ(preview.simplePreviewRegion, voxel::Region(pos, pos));
		}
	}

	modifier.shapeBrush().setRadius(2);
	for (int i = 0; i < 3; ++i) {
		const glm::ivec3 pos(i, 0, 0);
		modifier.setCursorPosition(pos, voxel::FaceNames::PositiveX);
		triggerPreviewUpdate(modifier, palette);
		const BrushPreview &preview = modifier.brushPreview();
		EXPECT_TRUE(preview.useSimplePreview);
		EXPECT_EQ(preview.simplePreviewRegion, voxel::Region(pos - 2, pos + 2));
	}
	modifier.shutdown();
	mgr.shutdown();
}

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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

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

	voxel::RawVolume *volume = mgr.volume(mgr.sceneGraph().activeNodeUUID());
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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

	triggerPreviewUpdate(modifier, palette);
	ASSERT_NE(modifier.previewVolume(), nullptr);

	modifier.resetPreview();
	EXPECT_EQ(modifier.previewVolume(), nullptr) << "After reset, preview volume should be nullptr";
	EXPECT_EQ(modifier.previewMirrorVolume(), nullptr) << "After reset, mirror volume should be nullptr";
	EXPECT_FALSE(modifier.brushPreview().useSimplePreview) << "After reset, simple preview should be false";
	modifier.shutdown();
	mgr.shutdown();
}

TEST_F(PreviewManagerTest, testErasePreviewShowsOnlyRemovedSourceVoxels) {
	core::SharedPtr<TrackingModifierRenderer> renderer = core::make_shared<TrackingModifierRenderer>();
	SceneManager mgr(core::make_shared<core::TimeProvider>(), _testApp->filesystem(),
					 core::make_shared<ISceneRenderer>(), core::make_shared<IModifierRenderer>());
	mgr.construct();
	ASSERT_TRUE(mgr.init());
	mgr.newScene(true, "test", voxel::Region(-10, 10));
	voxel::RawVolume *volume = mgr.volume(mgr.activeNodeUUID());
	ASSERT_NE(nullptr, volume);
	const voxel::Voxel source(voxel::VoxelType::Generic, 3);
	volume->setVoxel(0, 0, 0, source);
	volume->setVoxel(6, 0, 0, source);

	ModifierEx modifier(&mgr, renderer);
	modifier.construct();
	ASSERT_TRUE(modifier.init());

	command::Command::execute("shapeellipse");
	prepare(modifier, glm::ivec3(-2), glm::ivec3(2), ModifierType::Erase, BrushType::Shape);

	palette::Palette palette;
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

	triggerPreviewUpdate(modifier, palette);

	ASSERT_NE(modifier.previewVolume(), nullptr) << "Erase preview should still create a volume";
	const voxel::RawVolume *pv = modifier.previewVolume();
	const voxel::Region &pvRegion = pv->region();
	for (int z = pvRegion.getLowerZ(); z <= pvRegion.getUpperZ(); ++z) {
		for (int y = pvRegion.getLowerY(); y <= pvRegion.getUpperY(); ++y) {
			for (int x = pvRegion.getLowerX(); x <= pvRegion.getUpperX(); ++x) {
				EXPECT_EQ(x == 0 && y == 0 && z == 0, !voxel::isAir(pv->voxel(x, y, z).getMaterial()));
			}
		}
	}
	EXPECT_EQ(source.getColor(), pv->voxel(0, 0, 0).getColor());
	EXPECT_TRUE(pv->voxel(0, 0, 0).getFlags() & voxel::FlagOutline);
	EXPECT_EQ(source, volume->voxel(0, 0, 0));
	EXPECT_EQ(source, volume->voxel(6, 0, 0));
	EXPECT_TRUE(renderer->lastContext.previewErase);
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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

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
	palette.setColor(1, color::RGBA{255, 0, 0, 255});
	palette.setSize(2);

	triggerPreviewUpdate(modifier, palette);

	EXPECT_EQ(modifier.previewVolume(), nullptr);
	EXPECT_EQ(modifier.previewMirrorVolume(), nullptr);
	EXPECT_FALSE(modifier.brushPreview().useSimplePreview);
	modifier.shutdown();
	mgr.shutdown();
}

} // namespace voxedit
