/**
 * @file
 */
#include "TestMeshRenderer.h"
#include "testcore/TestAppMain.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/MeshCache.h"

TestMeshRenderer::TestMeshRenderer(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider),
		_meshRenderer(core::make_shared<voxelformat::MeshCache>()) {
	init(ORGANISATION, "testmeshrenderer");
}

app::AppState TestMeshRenderer::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return app::AppState::InitFailure;
	}
	if (!_meshRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return app::AppState::InitFailure;
	}
	_modelIndex = _meshRenderer.addMesh("assets/north-dir");
	if (_modelIndex == -1) {
		Log::error("Failed to load model");
		return app::AppState::InitFailure;
	}
	return state;
}

app::AppState TestMeshRenderer::onCleanup() {
	_meshRenderer.shutdown();
	return Super::onCleanup();
}

void TestMeshRenderer::doRender() {
	_meshRenderer.render(_modelIndex, camera());
}

TEST_APP(TestMeshRenderer)
