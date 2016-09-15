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
	shader::ColorShader _colorShader;

	int32_t _vertexBufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int32_t _colorBufferIndex = -1;

	void doRender() override;
public:
	TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
