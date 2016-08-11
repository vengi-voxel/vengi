#include "TestMesh.h"

TestMesh::TestMesh(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestMesh::onInit() {
	const core::AppState state = Super::onInit();
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 150.0f));
	_camera.lookAt(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setOmega(glm::vec3(0.0f, 0.001f, 0.0f));
	_camera.setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setTargetDistance(150.0f);
	_camera.setRotationType(video::CameraRotationType::Target);

	if (!_plane.init()) {
		return core::AppState::Cleanup;
	}

	_sunLight.init(glm::vec3(20.0f, 50.0f, -20.0), dimension());

	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::Cleanup;
	}

	const std::string mesh = "chr_skelett2_bake";
	_mesh = _meshPool.getMesh(mesh);
	if (!_mesh->isLoading()) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}

	return state;
}

void TestMesh::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
}

void TestMesh::doRender() {
	_plane.render(_camera);

	video::ScopedShader scoped(_meshShader);
	_meshShader.setView(_camera.viewMatrix());
	_meshShader.setProjection(_camera.projectionMatrix());
	_meshShader.setFogrange(2500.0f);
	_meshShader.setViewdistance(2500.0f);
	_meshShader.setModel(glm::mat4());
	_meshShader.setTexture(0);
	_meshShader.setDiffuseColor(_diffuseColor);
	_meshShader.setScreensize(glm::vec2(_camera.dimension()));
	_meshShader.setLight(_sunLight.modelViewProjectionMatrix());
	_meshShader.setShadowmap(1);

	// TODO: support more than just the first animation
	const uint8_t animationIndex = 0u;
	if (!_mesh->initMesh(_meshShader, (_now - _initTime) / 1000.0f, animationIndex)) {
		return;
	}
	core_assert_always(_mesh->render() > 0);
}

core::AppState TestMesh::onCleanup() {
	_meshShader.shutdown();
	_plane.shutdown();
	_mesh->shutdown();
	_meshPool.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestMesh>()->startMainLoop(argc, argv);
}
