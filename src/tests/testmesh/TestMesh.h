/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/MeshPool.h"
#include "TestmeshShaders.h"

class TestMesh: public TestApp {
private:
	using Super = TestApp;
	video::MeshPtr _mesh;
	video::MeshPool _meshPool;
	shader::MeshShader _meshShader;

	void doRender() override;
public:
	TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;

	void onMouseWheel(int32_t x, int32_t y) override;
};
