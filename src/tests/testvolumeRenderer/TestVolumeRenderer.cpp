/**
 * @file
 */

#include "TestVolumeRenderer.h"
#include "core/Log.h"
#include "palette/Palette.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedState.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

TestVolumeRenderer::TestVolumeRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _renderer(timeProvider) {
	init(ORGANISATION, "testvolumeRenderer");
	setCameraMotion(true);
	setRenderPlane(true);
	setRenderAxis(true);
}

app::AppState TestVolumeRenderer::onInit() {
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
		Log::error("Failed to init raw volume renderer");
		return app::AppState::InitFailure;
	}

	if (!_renderContext.init(windowDimension())) {
		Log::error("Failed to init render context");
		return app::AppState::InitFailure;
	}
	_renderContext.renderMode = voxelrender::RenderMode::Scene;

	_palette.nippon();

	const voxel::Region region(0, 0, 0, 31, 31, 31);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	// fill a sphere-like shape
	const glm::vec3 center = glm::vec3(region.getCenter());
	const float radius = 14.0f;
	const float radiusSq = radius * radius;
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				const glm::ivec3 pos(x, y, z);
				const glm::vec3 diff = glm::vec3(pos) - center;
				if (glm::dot(diff, diff) <= radiusSq) {
					const int colorIdx = 1 + (x + y + z) % (_palette.colorCount() - 1);
					volume->setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, colorIdx));
				}
			}
		}
	}

	const int idx = 0;
	delete _renderer.setVolume(_meshState, idx, volume, &_palette, nullptr, true);
	_renderer.scheduleRegionExtraction(_meshState, idx, region);

	camera().setMode(video::CameraMode::Perspective);
	camera().setType(video::CameraType::Free);
	camera().setTargetDistance(50.0f);
	camera().setWorldPosition(glm::vec3(16.0f, 30.0f, 50.0f));
	camera().lookAt(glm::vec3(16.0f, 16.0f, 16.0f));
	camera().setFarPlane(500.0f);
	camera().update(0.0);

	return state;
}

void TestVolumeRenderer::doRender() {
	video::ScopedState scopedDepth(video::State::DepthTest, true);
	video::ScopedState scopedCull(video::State::CullFace, true);
	video::ScopedState scopedDepthMask(video::State::DepthMask, true);
	video::ScopedState scopedBlend(video::State::Blend, true);

	_renderer.update(_meshState);
	_renderer.render(_meshState, _renderContext, camera(), true, false);
}

app::AppState TestVolumeRenderer::onCleanup() {
	if (_meshState) {
		_renderer.clear(_meshState);
		auto volumes = _meshState->shutdown();
		for (voxel::RawVolume *v : volumes) {
			delete v;
		}
	}
	_renderer.shutdown();
	_renderContext.shutdown();
	return Super::onCleanup();
}

TEST_APP(TestVolumeRenderer)
