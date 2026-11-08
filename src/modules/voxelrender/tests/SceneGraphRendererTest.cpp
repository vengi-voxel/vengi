/**
 * @file
 */

#include "voxelrender/SceneGraphRenderer.h"
#include "color/RGBA.h"
#include "core/ConfigVar.h"
#include "core/SharedPtr.h"
#include "core/TimeProvider.h"
#include "core/UUID.h"
#include "core/Var.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "video/Camera.h"
#include "video/tests/AbstractGLTest.h"
#include "voxel/MeshState.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelrender/RenderContext.h"
#include "voxelrender/RenderUtil.h"
#include "image/Image.h"

namespace voxelrender {

class TestableSceneGraphRenderer : public SceneGraphRenderer {
public:
	TestableSceneGraphRenderer(const core::TimeProviderPtr &timeProvider) : SceneGraphRenderer(timeProvider) {
	}
	using SceneGraphRenderer::prepare;
};

class SceneGraphRendererTest : public video::AbstractGLTest {
protected:
	voxel::MeshStatePtr _meshState;
	TestableSceneGraphRenderer _renderer;

	SceneGraphRendererTest() : _renderer(core::make_shared<core::TimeProvider>()) {
	}

	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
		_meshState = core::make_shared<voxel::MeshState>();
		_meshState->construct();
		ASSERT_TRUE(_meshState->init());
		_renderer.construct();
		ASSERT_TRUE(_renderer.init(false));
	}

	void TearDown() override {
		if (!IsSkipped()) {
			_renderer.shutdown();
			if (_meshState) {
				(void)_meshState->shutdown();
				_meshState = voxel::MeshStatePtr();
			}
		}
		Super::TearDown();
	}

private:
	using Super = video::AbstractGLTest;
};

TEST_F(SceneGraphRendererTest, InstancedReferenceShadowsMatchUniqueMeshesIncludingMirrors) {
	core::getVar(cfg::ClientShadowMap)->setVal(true);
	core::getVar(cfg::ClientDebugShadow)->setVal(false);
	core::getVar(cfg::ClientDebugShadowMapCascade)->setVal(false);
	_renderer.setSunAngle({45, 40, 0});
	palette::Palette palette;
	palette.minecraft();
	image::ImagePtr images[2];
	uint64_t calls[2]{};
	for (int variant = 0; variant < 2; ++variant) {
		scenegraph::SceneGraph scene;
		scenegraph::SceneGraphNode floor(scenegraph::SceneGraphNodeType::Model);
		floor.createVolume(voxel::Region(0, 0, 0, 23, 0, 23));
		floor.setPalette(palette);
		for (int z = 0; z < 24; ++z)
			for (int x = 0; x < 24; ++x)
				floor.volume()->setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		scene.emplace(core::move(floor));
		int source = -1;
		for (int i = 0; i < 8; ++i) {
			int id;
			if (variant == 0 && i > 0)
				id = scenegraph::createNodeReference(scene, scene.node(source));
			else {
				scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
				model.createVolume(voxel::Region(0, 0, 0, 1, 5, 2));
				model.setPalette(palette);
				for (int y = 0; y < 6; ++y)
					for (int z = 0; z < 3; ++z)
						for (int x = 0; x < 2; ++x)
							if (y < 2 || z == 0)
								model.volume()->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 2));
				id = scene.emplace(core::move(model));
				if (i == 0)
					source = id;
			}
			auto &transform = scene.node(id).transform();
			transform.setLocalTranslation({4 + (i % 4) * 5, 1, 6 + (i / 4) * 9});
			if (i % 2)
				transform.setLocalScale({-1, 1, 1});
		}
		scene.updateTransforms();
		RenderContext context;
		ASSERT_TRUE(context.init({128, 128}));
		context.sceneGraph = &scene;
		context.renderMode = RenderMode::Scene;
		context.onlyModels = true;
		video::Camera camera;
		camera.setSize({128, 128});
		camera.setNearPlane(.1f);
		camera.setFarPlane(100);
		camera.setWorldPosition({36, 30, 40});
		camera.lookAt({12, 0, 12});
		camera.update(0);
		video::clearColor({0, 0, 0, 1});
		context.beginFrame();
		const uint64_t before = video::renderStats().drawCalls;
		_renderer.render(_meshState, context, camera, true, true);
		calls[variant] = video::renderStats().drawCalls - before;
		context.endFrame();
		images[variant] = context.frameBuffer.image("reference shadows");
		ASSERT_TRUE(images[variant]);
		_renderer.clear(_meshState);
		context.shutdown();
	}
	EXPECT_LT(calls[0], calls[1]);
	int visible = 0;
	for (int y = 0; y < 128; ++y)
		for (int x = 0; x < 128; ++x) {
			const auto a = images[0]->colorAt(x, y);
			const auto b = images[1]->colorAt(x, y);
			visible += a.r > 10 || a.g > 10 || a.b > 10;
			EXPECT_NEAR(a.r, b.r, 1);
			EXPECT_NEAR(a.g, b.g, 1);
			EXPECT_NEAR(a.b, b.b, 1);
		}
	EXPECT_GT(visible, 1000);
}

TEST_F(SceneGraphRendererTest, BinaryTerrainAcrossMultipleChunksCoversItsWorldPosition) {
	core::getVar(cfg::VoxelMeshMode)->setVal((int)voxel::SurfaceExtractionType::Binary);
	core::getVar(cfg::ClientShadowMap)->setVal(false);
	_renderer.setAmbientColor(glm::vec3(1));
	_renderer.setDiffuseColor(glm::vec3(0));
	scenegraph::SceneGraph scene;
	scenegraph::SceneGraphNode floor(scenegraph::SceneGraphNodeType::Model);
	floor.createVolume(voxel::Region(0, 0, 0, 127, 0, 127));
	palette::Palette palette;
	palette.setColor(1, color::RGBA(77, 126, 68));
	floor.setPalette(palette);
	for (int z = 0; z < 128; ++z)
		for (int x = 0; x < 128; ++x)
			floor.volume()->setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	const int id = scene.emplace(core::move(floor));
	scene.updateTransforms();
	RenderContext context;
	ASSERT_TRUE(context.init({128, 128}));
	context.sceneGraph = &scene;
	context.renderMode = RenderMode::Scene;
	context.onlyModels = true;
	video::Camera camera;
	camera.setSize({128, 128});
	camera.setNearPlane(.1f);
	camera.setFarPlane(1000);
	camera.setWorldPosition({64, 160, 180});
	camera.lookAt({64, 0, 64});
	camera.update(0);
	for (int i = 0; i < 6; ++i) {
		context.beginFrame();
		_renderer.render(_meshState, context, camera, false, true);
		context.endFrame();
	}
	EXPECT_GT(_renderer.indexCount(scene.node(id).uuid(), voxel::MeshType_Opaque), 0u);
	auto image = context.frameBuffer.image("large terrain");
	ASSERT_TRUE(image);
	EXPECT_GT(image->colorAt(64, 64).g, 60);
	context.shutdown();
}

TEST_F(SceneGraphRendererTest, BlackEmissionDoesNotTintTheSceneWhenBloomIsEnabled) {
	for (int samples : {0, 4}) {
		core::getVar(cfg::ClientMultiSampleSamples)->setVal(samples);
		RenderContext context;
		ASSERT_TRUE(context.init(glm::ivec2(64)));
		video::clearColor(glm::vec4(0.125f, 0.25f, 0.375f, 1));
		context.beginFrame();
		context.resolveMultisampling();
		context.sceneHasGlow = true;
		context.applyBloom();
		context.beginOverlays();
		context.endFrame();
		const auto &display = context.enableMultisampling ? context.resolveFrameBuffer : context.frameBuffer;
		const auto image = display.image("bloom");
		ASSERT_TRUE(image);
		for (int y : {0, 32, 63}) {
			for (int x : {0, 32, 63}) {
				const auto color = image->colorAt(x, y);
				EXPECT_NEAR(32, color.r, 1) << "samples " << samples;
				EXPECT_NEAR(64, color.g, 1) << "samples " << samples;
				EXPECT_NEAR(96, color.b, 1) << "samples " << samples;
			}
		}
		context.shutdown();
	}
}

TEST_F(SceneGraphRendererTest, SceneBackgroundDoesNotPolluteBloomAttachment) {
	for (int samples : {0, 4}) {
		core::getVar(cfg::ClientMultiSampleSamples)->setVal(samples);
		RenderContext context;
		ASSERT_TRUE(context.init(glm::ivec2(32)));
		video::clearColor(glm::vec4(0.125f, 0.25f, 0.375f, 1));
		context.beginFrame();
		context.resolveMultisampling();
		context.beginOverlays();
		context.endFrame();
		const auto &display = context.enableMultisampling ? context.resolveFrameBuffer : context.frameBuffer;
		const auto glow = display.image("glow", video::FrameBufferAttachment::Color1);
		const auto scene = display.image("scene");
		ASSERT_TRUE(glow && scene);
		EXPECT_EQ(color::RGBA(0, 0, 0, 0), glow->colorAt(0, 0));
		EXPECT_NEAR(32, scene->colorAt(0, 0).r, 1);
		EXPECT_NEAR(64, scene->colorAt(0, 0).g, 1);
		EXPECT_NEAR(96, scene->colorAt(0, 0).b, 1);
		context.shutdown();
	}
}

TEST_F(SceneGraphRendererTest, testUuidVolumeIdxStableAndReused) {
	const core::UUID uuidA = core::UUID::generate();
	const core::UUID uuidB = core::UUID::generate();

	const int idxA = _renderer.getOrAssignVolumeIdx(uuidA);
	EXPECT_GE(idxA, 0);
	EXPECT_EQ(idxA, _renderer.getOrAssignVolumeIdx(uuidA));
	EXPECT_EQ(idxA, _renderer.getVolumeIdx(uuidA));

	const int idxB = _renderer.getOrAssignVolumeIdx(uuidB);
	EXPECT_GE(idxB, 0);
	EXPECT_NE(idxA, idxB);

	_renderer.nodeRemove(_meshState, uuidA);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuidA));
	EXPECT_EQ(idxB, _renderer.getVolumeIdx(uuidB));
	EXPECT_EQ(1, _renderer.freeVolumeIndexCount());

	const core::UUID uuidC = core::UUID::generate();
	const int idxC = _renderer.getOrAssignVolumeIdx(uuidC);
	EXPECT_EQ(idxA, idxC) << "freed compact volume index should be reused";
	EXPECT_EQ(0, _renderer.freeVolumeIndexCount());
}

TEST_F(SceneGraphRendererTest, testNodeRemoveWithoutVolumeIsSafe) {
	const core::UUID uuid = core::UUID::generate();
	const int idx = _renderer.getOrAssignVolumeIdx(uuid);
	EXPECT_GE(idx, 0);
	// Never called setVolume / scheduleRegionExtraction - deleteMesh path must tolerate unset buffers.
	_renderer.nodeRemove(_meshState, uuid);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuid));
	_renderer.nodeRemove(_meshState, uuid);
}

TEST_F(SceneGraphRendererTest, testPrepareDropsNodesMissingFromSceneGraph) {
	scenegraph::SceneGraph sceneA;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("a");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		model.setVolume(v);
		ASSERT_NE(InvalidNodeId, sceneA.emplace(core::move(model)));
	}
	sceneA.updateTransforms();

	RenderContext ctxA;
	ctxA.sceneGraph = &sceneA;
	ctxA.renderMode = RenderMode::Scene;
	_renderer.prepare(_meshState, ctxA);
	const core::UUID uuidA = sceneA.firstModelNode()->uuid();
	EXPECT_GE(_renderer.getVolumeIdx(uuidA), 0);

	scenegraph::SceneGraph sceneB;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("b");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		model.setVolume(v);
		ASSERT_NE(InvalidNodeId, sceneB.emplace(core::move(model)));
	}
	sceneB.updateTransforms();

	RenderContext ctxB;
	ctxB.sceneGraph = &sceneB;
	ctxB.renderMode = RenderMode::Scene;
	_renderer.prepare(_meshState, ctxB);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuidA)) << "uuid volume slots from the old scene must not survive prepare";
	EXPECT_GE(_renderer.getVolumeIdx(sceneB.firstModelNode()->uuid()), 0);
}

TEST_F(SceneGraphRendererTest, testClearResetsUuidMapping) {
	const core::UUID uuid = core::UUID::generate();
	EXPECT_GE(_renderer.getOrAssignVolumeIdx(uuid), 0);
	_renderer.clear(_meshState);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuid));
	EXPECT_EQ(0, _renderer.mappedNodeCount());
	EXPECT_EQ(0, _renderer.nextVolumeIdx());
}

TEST_F(SceneGraphRendererTest, testModelReferenceLinksToTargetMeshSlot) {
	scenegraph::SceneGraph sceneGraph;
	int modelId = InvalidNodeId;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("model");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		model.setVolume(v);
		modelId = sceneGraph.emplace(core::move(model));
		ASSERT_NE(InvalidNodeId, modelId);
	}
	const int refId = scenegraph::createNodeReference(sceneGraph, sceneGraph.node(modelId));
	ASSERT_NE(InvalidNodeId, refId);
	sceneGraph.updateTransforms();

	RenderContext renderContext;
	renderContext.sceneGraph = &sceneGraph;
	renderContext.renderMode = RenderMode::Scene;
	_renderer.prepare(_meshState, renderContext);

	const scenegraph::SceneGraphNode &model = sceneGraph.node(modelId);
	const scenegraph::SceneGraphNode &ref = sceneGraph.node(refId);
	const int modelIdx = _renderer.getVolumeIdx(model);
	const int refIdx = _renderer.getVolumeIdx(ref);
	ASSERT_GE(modelIdx, 0);
	ASSERT_GE(refIdx, 0);
	EXPECT_NE(modelIdx, refIdx);
	EXPECT_EQ(modelIdx, _meshState->reference(refIdx));
	EXPECT_EQ(-1, _meshState->reference(modelIdx));
}

TEST_F(SceneGraphRendererTest, testRenderTransparentWeightedOIT) {
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
	model.setName("model");
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
	palette::Palette pal;
	pal.nippon();
	pal.setColor(2, color::RGBA(40, 80, 200, 128));
	pal.setMaterialType(2, palette::MaterialType::Glass);
	v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v->setVoxel(3, 3, 3, voxel::createVoxel(voxel::VoxelType::Transparent, 2));
	v->setVoxel(4, 3, 3, voxel::createVoxel(voxel::VoxelType::Transparent, 2));
	model.setVolume(v);
	model.setPalette(pal);
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(model)));
	sceneGraph.updateTransforms();

	RenderContext renderContext;
	ASSERT_TRUE(renderContext.init(glm::ivec2(64, 64)));
	renderContext.sceneGraph = &sceneGraph;
	renderContext.renderMode = RenderMode::Scene;
	renderContext.enableBloom = false;
	EXPECT_TRUE(renderContext.hasOit());

	video::Camera camera;
	camera.setSize(glm::ivec2(64, 64));
	configureCamera(camera, sceneGraph.sceneRegion(), SceneCameraMode::Free, 500.0f);

	renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, renderContext, camera, false, true);
	renderContext.frameBuffer.unbind();
	renderContext.shutdown();
}

TEST_F(SceneGraphRendererTest, testBloomThenOverlays) {
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
	model.setName("model");
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
	palette::Palette pal;
	pal.nippon();
	pal.setEmit(1, 1.0f);
	v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	model.setVolume(v);
	model.setPalette(pal);
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(model)));
	sceneGraph.updateTransforms();

	RenderContext renderContext;
	ASSERT_TRUE(renderContext.init(glm::ivec2(64, 64)));
	renderContext.sceneGraph = &sceneGraph;
	renderContext.renderMode = RenderMode::Scene;
	renderContext.enableBloom = true;

	video::Camera camera;
	camera.setSize(glm::ivec2(64, 64));
	configureCamera(camera, sceneGraph.sceneRegion(), SceneCameraMode::Free, 500.0f);

	renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, renderContext, camera, false, true);
	renderContext.resolveMultisampling();
	renderContext.applyBloom();
	renderContext.beginOverlays();
	renderContext.endFrame();
	renderContext.shutdown();
}

TEST_F(SceneGraphRendererTest, testClearAfterRenderDropsVolumesSafely) {
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
	model.setName("model");
	voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 15, 15, 15));
	v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	model.setVolume(v);
	ASSERT_NE(InvalidNodeId, sceneGraph.emplace(core::move(model)));
	sceneGraph.updateTransforms();

	RenderContext renderContext;
	ASSERT_TRUE(renderContext.init(glm::ivec2(64, 64)));
	renderContext.sceneGraph = &sceneGraph;
	renderContext.renderMode = RenderMode::Scene;
	renderContext.enableBloom = false;

	video::Camera camera;
	camera.setSize(glm::ivec2(64, 64));
	configureCamera(camera, sceneGraph.sceneRegion(), SceneCameraMode::Free, 500.0f);

	renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, renderContext, camera, false, true);
	renderContext.frameBuffer.unbind();

	// render() builds cull lists on the calling thread. Drop renderer pointers
	// before the scene graph frees the volumes.
	_renderer.clear(_meshState);
	sceneGraph.clear();
	renderContext.shutdown();
}

TEST_F(SceneGraphRendererTest, testNewOpaqueSceneDropsPreviousTransparencyMesh) {
	scenegraph::SceneGraph glassScene;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("glass");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
		palette::Palette pal;
		pal.nippon();
		pal.setColor(2, color::RGBA(40, 80, 200, 128));
		pal.setMaterialType(2, palette::MaterialType::Glass);
		v->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		for (int x = 0; x < 8; ++x) {
			for (int z = 0; z < 8; ++z) {
				v->setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Transparent, 2));
			}
		}
		model.setVolume(v);
		model.setPalette(pal);
		ASSERT_NE(InvalidNodeId, glassScene.emplace(core::move(model)));
	}
	glassScene.updateTransforms();
	const core::UUID glassUuid = glassScene.firstModelNode()->uuid();

	RenderContext renderContext;
	ASSERT_TRUE(renderContext.init(glm::ivec2(64, 64)));
	renderContext.renderMode = RenderMode::Scene;
	renderContext.enableBloom = false;

	video::Camera camera;
	camera.setSize(glm::ivec2(64, 64));
	configureCamera(camera, glassScene.sceneRegion(), SceneCameraMode::Top, 500.0f);
	camera.update(0.0);

	renderContext.sceneGraph = &glassScene;
	renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, renderContext, camera, false, true);
	EXPECT_GT(_renderer.indexCount(glassUuid, voxel::MeshType_Transparency), 0u);

	scenegraph::SceneGraph opaqueScene;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("opaque");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0, 0, 7, 7, 7));
		v->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		model.setVolume(v);
		ASSERT_NE(InvalidNodeId, opaqueScene.emplace(core::move(model)));
	}
	opaqueScene.updateTransforms();
	const core::UUID opaqueUuid = opaqueScene.firstModelNode()->uuid();

	renderContext.sceneGraph = &opaqueScene;
	renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, renderContext, camera, false, true);
	renderContext.frameBuffer.unbind();

	EXPECT_EQ(-1, _renderer.getVolumeIdx(glassUuid));
	EXPECT_EQ(0u, _renderer.indexCount(glassUuid, voxel::MeshType_Transparency));
	EXPECT_EQ(0u, _renderer.indexCount(opaqueUuid, voxel::MeshType_Transparency));
	EXPECT_GT(_renderer.indexCount(opaqueUuid, voxel::MeshType_Opaque), 0u);

	renderContext.shutdown();
}

} // namespace voxelrender
