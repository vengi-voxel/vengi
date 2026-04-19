/**
 * @file
 */

#include "TestSceneGraphRenderer.h"
#include "core/Log.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedState.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelrender/RenderUtil.h"

TestSceneGraphRenderer::TestSceneGraphRenderer(const io::FilesystemPtr &filesystem,
											   const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _renderer(timeProvider) {
	init(ORGANISATION, "testscenegraphrenderer");
	setCameraMotion(true);
	setRenderPlane(true);
	setRenderAxis(true);
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
		const glm::vec3 center = glm::vec3(region.getCenter());
		const float radius = 14.0f;
		const float radiusSq = radius * radius;
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
					const glm::ivec3 pos(x, y, z);
					const glm::vec3 diff = glm::vec3(pos) - center;
					if (glm::dot(diff, diff) <= radiusSq) {
						const int colorIdx = 1 + (x + y + z) % 254;
						volume->setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, colorIdx));
					}
				}
			}
		}

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume);
		node.setName("sphere");
		palette::Palette pal;
		pal.nippon();
		node.setPalette(pal);
		_sceneGraph.emplace(core::move(node));
	}

	// create a second model node - a cube, offset to the side
	{
		const voxel::Region region(0, 0, 0, 15, 15, 15);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
					volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 2));
				}
			}
		}

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume);
		node.setName("cube");
		palette::Palette pal;
		pal.nippon();
		node.setPalette(pal);
		scenegraph::SceneGraphTransform transform;
		transform.setLocalTranslation(glm::vec3(40.0f, 0.0f, 0.0f));
		const scenegraph::FrameIndex frameIdx = 0;
		node.setTransform(frameIdx, transform);
		_sceneGraph.emplace(core::move(node));
	}

	const voxel::Region sceneRegion = _sceneGraph.sceneRegion();
	voxelrender::configureCamera(camera(), sceneRegion, voxelrender::SceneCameraMode::Free, 500.0f);
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
