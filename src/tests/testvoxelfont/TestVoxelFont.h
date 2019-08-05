/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelfont/VoxelFont.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "video/Buffer.h"
#include "RenderShaders.h"

class TestVoxelFont: public TestApp {
private:
	using Super = TestApp;

	voxel::VoxelFont _voxelFont;
	voxelrender::RawVolumeRenderer _rawVolumeRenderer;

	int _fontSize = 20;
	int _thickness = 4;
	bool _mergeQuads = false;
	bool _upperLeft = false;

	int _vertices = 0;
	int _indices = 0;

	void doRender() override;

	bool changeFontSize(int delta);
public:
	TestVoxelFont(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void onRenderUI() override;

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	bool onMouseWheel(int32_t x, int32_t y) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
