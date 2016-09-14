/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "frontend/font/VoxelFont.h"

class TestVoxelFont: public TestApp {
private:
	using Super = TestApp;

	frontend::VoxelFont _voxelFont;

	void doRender() override;
public:
	TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
