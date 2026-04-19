/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxel/MeshState.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/RenderContext.h"

class TestVolumeRenderer : public TestApp {
private:
	using Super = TestApp;
	voxelrender::RawVolumeRenderer _renderer;
	voxel::MeshStatePtr _meshState;
	voxelrender::RenderContext _renderContext;
	palette::Palette _palette;

	void doRender() override;

public:
	TestVolumeRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
