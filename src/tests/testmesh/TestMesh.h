/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/Mesh.h"
#include "TestmeshShaders.h"

class TestMesh: public TestApp {
private:
	using Super = TestApp;
	video::Mesh _mesh;
	shader::MeshShader _meshShader;

	void doRender() override;
public:
	TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
};
