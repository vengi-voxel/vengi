/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Mesh.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "TestmeshShaders.h"

class TestMesh: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	video::Mesh _mesh;
	video::Camera _camera;
	shader::MeshShader _meshShader;
	frontend::Axis _axis;
	uint8_t _moveMask = 0;
public:
	TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~TestMesh();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};
