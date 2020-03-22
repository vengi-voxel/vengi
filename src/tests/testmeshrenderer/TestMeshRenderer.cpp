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
		_meshRenderer(std::make_shared<voxelformat::MeshCache>()) {
	init(ORGANISATION, "testmeshrenderer");
}

core::AppState TestMeshRenderer::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}
	if (!_meshRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::InitFailure;
	}
	_modelIndex = _meshRenderer.addMesh("assets/north-dir.vox");
	if (_modelIndex == -1) {
		Log::error("Failed to load model");
		return core::AppState::InitFailure;
	}
	return state;
}

core::AppState TestMeshRenderer::onCleanup() {
	core::AppState state = Super::onCleanup();
	_meshRenderer.shutdown();
	return state;
}

void TestMeshRenderer::doRender() {
	_meshRenderer.render(_modelIndex, camera());
}

TEST_APP(TestMeshRenderer)
