/**
 * @file
 */

#include "voxedit-util/SceneRenderer.h"
#include "scenegraph/SceneGraph.h"
#include "video/Camera.h"
#include "video/tests/AbstractGLTest.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/modifier/ModifierRenderer.h"
#include "voxel/Region.h"
#include "voxelrender/RenderContext.h"

namespace voxedit {

class SceneRendererTest : public video::AbstractGLTest {
private:
	using Super = video::AbstractGLTest;

protected:
	SceneRenderer _sceneRenderer;

	SceneRendererTest() : _sceneRenderer(core::make_shared<core::TimeProvider>()) {
	}

	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
		const core::VarDef voxEditShowgrid(cfg::VoxEditShowgrid, true, "", "");
		core::Var::registerVar(voxEditShowgrid);
		const core::VarDef voxEditShowlockedaxis(cfg::VoxEditShowlockedaxis, true, "", "");
		core::Var::registerVar(voxEditShowlockedaxis);
		const core::VarDef voxEditRendershadow(cfg::VoxEditRendershadow, true, "", "");
		core::Var::registerVar(voxEditRendershadow);
		const core::VarDef voxEditGridsize(cfg::VoxEditGridsize, "1", "", "");
		core::Var::registerVar(voxEditGridsize);
		const core::VarDef voxEditPlaneSize(cfg::VoxEditPlaneSize, 0, "", "");
		core::Var::registerVar(voxEditPlaneSize);
		const core::VarDef voxEditShowPlane(cfg::VoxEditShowPlane, false, "", "");
		core::Var::registerVar(voxEditShowPlane);
		const core::VarDef voxRenderMeshMode(cfg::VoxelMeshMode, (int)voxel::SurfaceExtractionType::Binary, "", "");
		core::Var::registerVar(voxRenderMeshMode);
		const core::VarDef voxEditShowaabb(cfg::VoxEditShowaabb, "", "", "");
		core::Var::registerVar(voxEditShowaabb);
		const core::VarDef voxEditShowBones(cfg::VoxEditShowBones, "", "", "");
		core::Var::registerVar(voxEditShowBones);
		const core::VarDef voxEditGrayInactive(cfg::VoxEditGrayInactive, "", "", "");
		core::Var::registerVar(voxEditGrayInactive);
		const core::VarDef voxEditHideInactive(cfg::VoxEditHideInactive, "", "", "");
		core::Var::registerVar(voxEditHideInactive);
		_sceneRenderer.construct();
		ASSERT_TRUE(_sceneRenderer.init());
	};

	void TearDown() override {
		if (!IsSkipped()) {
			_sceneRenderer.shutdown();
		}
		Super::TearDown();
	}
};

TEST_F(SceneRendererTest, testInit) {
	// empty to test init and shutdown
}

TEST_F(SceneRendererTest, testUnchangedBrushPreviewDoesNotQueueExtractionAgain) {
	const voxel::MeshStatePtr meshState = core::make_shared<voxel::MeshState>();
	ModifierRenderer renderer(core::make_shared<core::TimeProvider>(), meshState);
	ASSERT_TRUE(renderer.init());
	voxel::RawVolume volume(voxel::Region(0, 7));
	volume.setVoxel(1, 1, 1, voxel::Voxel(voxel::VoxelType::Generic, 1));
	palette::Palette palette;
	palette.nippon();
	ModifierRendererContext context;
	context.brushActive = true;
	context.activeRegion = volume.region();
	context.previewVolume = &volume;
	context.palette = &palette;
	renderer.update(context);
	const int pending = meshState->pendingExtractions();
	EXPECT_GT(pending, 0);
	renderer.update(context);
	EXPECT_EQ(pending, meshState->pendingExtractions());
	renderer.waitForPendingExtractions();
	renderer.update(context);
	EXPECT_EQ(0, meshState->pendingExtractions());
	renderer.clearBrushVolumes();
	renderer.shutdown();
}

TEST_F(SceneRendererTest, testRemoveNodeCancelsPendingRegionExtraction) {
	const core::UUID nodeUUID = core::UUID::generate();
	const voxel::Region region(0, 0, 0, 7, 7, 7);
	// Queue an extraction, then remove the node before the command buffer is flushed.
	_sceneRenderer.updateNodeRegion(nodeUUID, region);
	_sceneRenderer.removeNode(nodeUUID);
	_sceneRenderer.update();
}

TEST_F(SceneRendererTest, testClearCancelsPendingRegionExtractions) {
	const voxel::Region region(0, 0, 0, 7, 7, 7);
	_sceneRenderer.updateNodeRegion(core::UUID::generate(), region);
	_sceneRenderer.updateNodeRegion(core::UUID::generate(), region);
	_sceneRenderer.clear();
	EXPECT_EQ(0, _sceneRenderer.rendererStats().pendingExtractions);
	EXPECT_EQ(0, _sceneRenderer.rendererStats().mappedNodes);
	_sceneRenderer.update();
	EXPECT_EQ(0, _sceneRenderer.rendererStats().pendingExtractions);
}

TEST_F(SceneRendererTest, testClearBeforeRenderReleasesBorrowedSceneDataWithoutAnUpdate) {
	scenegraph::SceneGraph scene;
	const core::UUID uuid = core::UUID::generate();
	const voxel::Region region(0, 7);
	const auto addModel = [&]() {
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model, uuid);
		model.createVolume(region);
		model.palette().nippon();
		const glm::vec3 normal[] = {glm::vec3(0, 1, 0)};
		model.normalPalette().loadNormalMap(normal, lengthof(normal));
		const int id = scene.emplace(core::move(model));
		scene.setActiveNode(id);
		scene.updateTransforms();
	};
	addModel();
	voxelrender::RenderContext context;
	ASSERT_TRUE(context.init(glm::ivec2(64)));
	context.sceneGraph = &scene;
	video::Camera camera;
	camera.setSize(glm::ivec2(64));
	camera.setNearPlane(0.1f);
	camera.setFarPlane(100);
	camera.setWorldPosition(glm::vec3(16, 16, 16));
	camera.lookAt(glm::vec3(4));
	camera.update(0);
	context.beginFrame();
	_sceneRenderer.renderScene(context, camera);
	context.endFrame();
	EXPECT_EQ(1, _sceneRenderer.rendererStats().mappedNodes);

	// UI actions can replace a scene between update() and renderScene().
	_sceneRenderer.updateNodeRegion(uuid, region);
	_sceneRenderer.clear();
	scene.clear();
	addModel();
	context.beginFrame();
	_sceneRenderer.renderScene(context, camera);
	context.endFrame();
	EXPECT_EQ(1, _sceneRenderer.rendererStats().mappedNodes);
	_sceneRenderer.clear();
	_sceneRenderer.update();
	context.shutdown();
}

} // namespace voxedit
