#include "TestMesh.h"

TestMesh::TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestMesh::onInit() {
	const core::AppState state = Super::onInit();
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 150.0f));
	_camera.lookAt(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setOmega(glm::vec3(0.0f, 0.1f, 0.0f));
	_camera.setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setTargetDistance(150.0f);
	_camera.setRotationType(video::CameraRotationType::Target);

	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::Cleanup;
	}

	const std::string mesh = "chr_skelett2_bake";
	if (!_mesh.loadMesh(mesh)) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}

	return state;
}

void TestMesh::doRender() {
	video::ScopedShader scoped(_meshShader);
	_meshShader.setView(_camera.viewMatrix());
	_meshShader.setProjection(_camera.projectionMatrix());
	_meshShader.setFogrange(2500.0f);
	_meshShader.setViewdistance(2500.0f);
	_meshShader.setModel(glm::mat4());
	_meshShader.setTexture(0);

	// TODO: support more than just the first animation
	const uint8_t animationIndex = 0u;
	if (!_mesh.initMesh(_meshShader, (_now - _initTime) / 1000.0f, animationIndex)) {
		Log::error("Failed to init the mesh");
		return;
	}
	core_assert_always(_mesh.render() > 0);
}

core::AppState TestMesh::onCleanup() {
	_meshShader.shutdown();
	_mesh.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestMesh>()->startMainLoop(argc, argv);
}
