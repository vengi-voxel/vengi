/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelformat/MeshCache.h"
#include "voxelrender/CachedMeshRenderer.h"

class TestMeshRenderer: public TestApp {
private:
	using Super = TestApp;
	voxelrender::CachedMeshRenderer _meshRenderer;
	int _modelIndex = -1;

	void doRender() override;
public:
	TestMeshRenderer(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
