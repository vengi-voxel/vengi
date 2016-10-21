/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxel/font/VoxelFont.h"
#include "video/VertexBuffer.h"
#include "FrontendShaders.h"

class TestVoxelFont: public TestApp {
private:
	using Super = TestApp;

	voxel::VoxelFont _voxelFont;
	video::VertexBuffer _vertexBuffer;
	shader::ColorShader& _colorShader;

	int32_t _vertexBufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int32_t _colorBufferIndex = -1;

	int _fontSize = 20;

	void doRender() override;

	bool changeFontSize(int delta);
public:
	TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void afterUI() override;

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	void onMouseWheel(int32_t x, int32_t y) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
};
