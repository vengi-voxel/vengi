#include "TestMesh.h"
#include "core/AppModule.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "core/Command.h"
#include "frontend/Movement.h"

TestMesh::TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus, 21000) {
	init("engine", "TestMesh");
}

TestMesh::~TestMesh() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

core::AppState TestMesh::onInit() {
	const core::AppState state = Super::onInit();

	GLDebug::enable(GLDebug::Medium);

	_axis.init();

	_camera.init(_width, _height);
	_camera.setPosition(glm::vec3(50.0f, 50.0f, 0.0f));
	_camera.lookAt(glm::vec3(0.0f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::Cleanup;
	}

	const std::string mesh = "animal_chicken.dae";
	if (!_mesh.loadMesh(mesh)) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}

	const glm::vec4& color = ::core::Color::Red;
	glClearColor(color.r, color.g, color.b, color.a);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	return state;
}

core::AppState TestMesh::onRunning() {
	const core::AppState state = Super::onRunning();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_camera.setFarPlane(500.0f);
	_camera.setFieldOfView(45.0f);
	_camera.setAspectRatio(_aspect);
	const bool left = _moveMask & MOVELEFT;
	const bool right = _moveMask & MOVERIGHT;
	const bool forward = _moveMask & MOVEFORWARD;
	const bool backward = _moveMask & MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	_camera.update();

	{
		video::ScopedShader scoped(_meshShader);
		_meshShader.setView(_camera.viewMatrix());
		_meshShader.setProjection(_camera.projectionMatrix());
		_meshShader.setFogrange(500.0f);
		_meshShader.setViewdistance(500.0f);
		_meshShader.setModel(glm::mat4());
		_meshShader.setTexture(0);

		if (!_mesh.initMesh(_meshShader)) {
			Log::error("Failed to init the mesh");
			return core::AppState::Cleanup;
		}
		core_assert(_mesh.render() > 0);
	}

	_axis.render(_camera);

	return state;
}

core::AppState TestMesh::onCleanup() {
	_meshShader.shutdown();
	_mesh.shutdown();
	_axis.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestMesh>()->startMainLoop(argc, argv);
}
