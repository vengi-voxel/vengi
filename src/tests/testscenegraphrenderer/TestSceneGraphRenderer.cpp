/**
 * @file
 */

#include "TestSceneGraphRenderer.h"
#include "core/Log.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphUtil.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedState.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelrender/RenderUtil.h"
#include "math/Quatf.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"


TestSceneGraphRenderer::TestSceneGraphRenderer(const io::FilesystemPtr &filesystem,
											   const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _renderer(timeProvider) {
	init(ORGANISATION, "testscenegraphrenderer");
	setCameraMotion(true);
	setRenderPlane(true);
	setRenderAxis(true);
	registerArg("--shadow-close").setDescription("Inspect contact shadows close to the first casters").addFlag(ARGUMENT_FLAG_BOOL);
	registerArg("--shadow-grid").setDescription("Render a grid of shadow casters (side length, up to 32)");
}

app::AppState TestSceneGraphRenderer::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	_meshState = core::make_shared<voxel::MeshState>();
	_meshState->construct();
	if (!_meshState->init()) {
		Log::error("Failed to init mesh state");
		return app::AppState::InitFailure;
	}

	_renderer.construct();
	if (!_renderer.init(_meshState->hasNormals())) {
		Log::error("Failed to init scene graph renderer");
		return app::AppState::InitFailure;
	}

	if (!_renderContext.init(windowDimension())) {
		Log::error("Failed to init render context");
		return app::AppState::InitFailure;
	}
	_renderContext.renderMode = voxelrender::RenderMode::Scene;
	_renderContext.sceneGraph = &_sceneGraph;
	_renderContext.onlyModels = true;

	// create the first model node - a sphere
	{
		const voxel::Region region(0, 0, 0, 31, 31, 31);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		const math::Vec3i center = region.getCenter();
		const int diameter = 29;
		const math::Vec3i centerBottom(center.x, region.getLowerY(), center.z);
		voxelgenerator::shape::createEllipse(*volume, centerBottom, math::Axis::Y, diameter, diameter, diameter,
											 voxel::createVoxel(voxel::VoxelType::Generic, 1));

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume);
		node.setName("sphere");
		palette::Palette pal;
		pal.nippon();
		node.setPalette(pal);
		scenegraph::SceneGraphTransform transform;
		const math::Quatf orientation =
			math::angleAxis(math::radians(25.0f), math::Vec3f(1.0f, 0.0f, 0.0f)) *
			math::angleAxis(math::radians(35.0f), math::Vec3f(0.0f, 1.0f, 0.0f)) *
			math::angleAxis(math::radians(15.0f), math::Vec3f(0.0f, 0.0f, 1.0f));
		transform.setLocalOrientation(orientation);
		const scenegraph::FrameIndex frameIdx = 0;
		node.setTransform(frameIdx, transform);
		_sceneGraph.emplace(core::move(node));
	}

	// create a second model node - a cube, offset to the side
	{
		const voxel::Region region(0, 0, 0, 15, 15, 15);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		voxelgenerator::shape::createCubeNoCenter(*volume, region.getLowerCorner(), region.getWidthInVoxels(),
												  region.getHeightInVoxels(), region.getDepthInVoxels(),
												  voxel::createVoxel(voxel::VoxelType::Generic, 2));

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume);
		node.setName("cube");
		palette::Palette pal;
		pal.nippon();
		node.setPalette(pal);
		scenegraph::SceneGraphTransform transform;
		transform.setLocalTranslation(math::Vec3f(40.0f, 0.0f, 0.0f));
		const math::Quatf orientation =
			math::angleAxis(math::radians(20.0f), math::Vec3f(1.0f, 0.0f, 0.0f)) *
			math::angleAxis(math::radians(30.0f), math::Vec3f(0.0f, 1.0f, 0.0f)) *
			math::angleAxis(math::radians(10.0f), math::Vec3f(0.0f, 0.0f, 1.0f));
		transform.setLocalOrientation(orientation);
		const scenegraph::FrameIndex frameIdx = 0;
		node.setTransform(frameIdx, transform);
		_sceneGraph.emplace(core::move(node));
	}

	if (hasArg("--shadow-grid") || hasArg("--shadow-close")) {
		core::getVar(cfg::ClientShadowMap)->setVal(true);
		const int side = math::clamp(getArgVal("--shadow-grid", "8").toInt(), 1, 32);
		for (int z = 0; z < side; ++z) {
			for (int x = 0; x < side; ++x) {
				const auto &source = _sceneGraph.node(1 + (x + z) % 2);
				const int id = z % 2 ? scenegraph::copyNodeToSceneGraph(_sceneGraph, source, 0)
									: scenegraph::createNodeReference(_sceneGraph, source);
				auto &transform = _sceneGraph.node(id).transform();
				transform.setLocalTranslation({x * 40.0f, 0.0f, z * 40.0f});
				if (x % 2) {
					transform.setLocalScale({-1.0f, 1.0f, 1.0f});
				}
			}
		}
		scenegraph::SceneGraphNode floor(scenegraph::SceneGraphNodeType::Model);
		floor.createVolume(voxel::Region(-40, -2, -40, side * 40, -1, side * 40));
		voxelgenerator::shape::createCubeNoCenter(*floor.volume(), floor.region().getLowerCorner(),
			floor.region().getWidthInVoxels(), 2, floor.region().getDepthInVoxels(),
			voxel::createVoxel(voxel::VoxelType::Generic, 3));
		palette::Palette pal;
		pal.nippon();
		floor.setPalette(pal);
		_sceneGraph.emplace(core::move(floor));
	}

	_sceneGraph.updateTransforms();

	const voxel::Region sceneRegion = _sceneGraph.sceneRegion();
	voxelrender::configureCamera(camera(), sceneRegion, voxelrender::SceneCameraMode::Free, 500.0f);
	if (hasArg("--shadow-close")) {
		setRenderPlane(false);
		setRenderAxis(false);
		camera().setRotationType(video::CameraRotationType::Eye);
		camera().setNearPlane(0.1f);
		camera().setTargetDistance(100.0f);
		camera().setWorldPosition({65.0f, 6.0f, 65.0f});
		camera().lookAt({85.0f, 1.0f, 85.0f});
		_renderer.setSunAngle({35.0f, 90.3f, 0.0f});
	}
	camera().update(0.0);

	return state;
}

void TestSceneGraphRenderer::doRender() {
	video::ScopedState scopedDepth(video::State::DepthTest, true);
	video::ScopedState scopedCull(video::State::CullFace, true);
	video::ScopedState scopedDepthMask(video::State::DepthMask, true);
	video::ScopedState scopedBlend(video::State::Blend, true);

	_renderer.render(_meshState, _renderContext, camera(), true, true);
}

app::AppState TestSceneGraphRenderer::onCleanup() {
	_renderer.clear(_meshState);
	_renderer.shutdown();
	_renderContext.shutdown();
	if (_meshState) {
		(void)_meshState->shutdown();
	}
	return Super::onCleanup();
}

TEST_APP(TestSceneGraphRenderer)
