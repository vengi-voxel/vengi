#include "TestDepthBuffer.h"
#include "video/ScopedViewPort.h"

TestDepthBuffer::TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestDepthBuffer::onInit() {
	core::AppState state = Super::onInit();

	if (!_plane.init()) {
		return core::AppState::Cleanup;
	}

	_sunLight.init(glm::vec3(20.0f, 50.0f, -20.0), dimension());
	_camera.setPosition(glm::vec3(0.0f, 10.0f, 150.0f));
	_camera.setOmega(glm::vec3(0.0f, 0.001f, 0.0f));
	_camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	_camera.setTargetDistance(50.0f);
	_camera.setRotationType(video::CameraRotationType::Target);

	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmaprender shader");
		return core::AppState::Cleanup;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return core::AppState::Cleanup;
	}
	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::Cleanup;
	}

	const glm::ivec2& fullscreenQuadIndices = _texturedFullscreenQuad.createFullscreenTexturedQuad();
	_texturedFullscreenQuad.addAttribute(_shadowMapRenderShader.getLocationPos(), fullscreenQuadIndices.x, 3);
	_texturedFullscreenQuad.addAttribute(_shadowMapRenderShader.getLocationTexcoord(), fullscreenQuadIndices.y, 2);

	const std::string mesh = "chr_skelett2_bake";
	_mesh = _meshPool.getMesh(mesh);
	if (!_mesh->isLoading()) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}
	if (!_depthBuffer.init(_dimension)) {
		Log::error("Failed to init the depthbuffer");
		return core::AppState::Cleanup;
	}

	return state;
}

void TestDepthBuffer::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
}

void TestDepthBuffer::doRender() {
	_sunLight.update(_deltaFrame, _camera);
	// TODO: support different animations...
	const uint8_t animationIndex = 0u;
	const long timeInSeconds = (_now - _initTime) / 1000.0f;
	{
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setLight(_sunLight.modelViewProjectionMatrix());
		_shadowMapShader.setModel(glm::mat4());
		if (_mesh->initMesh(_shadowMapShader, timeInSeconds, animationIndex)) {
			glDisable(GL_BLEND);
			glCullFace(GL_FRONT);
			_depthBuffer.bind();
			_mesh->render();
			_depthBuffer.unbind();
			glCullFace(GL_BACK);
			glEnable(GL_BLEND);
		}
	}
	{
		glClearColor(0.8, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderPlane();

		video::ScopedShader scoped(_meshShader);
		_meshShader.setView(_camera.viewMatrix());
		_meshShader.setProjection(_camera.projectionMatrix());
		_meshShader.setFogrange(500.0f);
		_meshShader.setViewdistance(500.0f);
		_meshShader.setModel(glm::mat4());
		_meshShader.setLightpos(_sunLight.direction() + _camera.position());
		_meshShader.setTexture(0);

		if (_mesh->initMesh(_meshShader, timeInSeconds, animationIndex)) {
			core_assert_always(_mesh->render() > 0);
		}
	}
	{
		const int width = _camera.width();
		const int height = _camera.height();
		const GLsizei quadWidth = (GLsizei) (width / 3.0f);
		const GLsizei quadHeight = (GLsizei) (height / 3.0f);
		video::ScopedShader scopedShader(_shadowMapRenderShader);
		video::ScopedViewPort scopedViewport(width - quadWidth, 0, quadWidth, quadHeight);
		core_assert_always(_texturedFullscreenQuad.bind());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture());
		_shadowMapRenderShader.setShadowmap(0);
		glDrawArrays(GL_TRIANGLES, 0, _texturedFullscreenQuad.elements(0));
		_texturedFullscreenQuad.unbind();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void TestDepthBuffer::renderPlane() {
	_plane.render(_camera);
}

core::AppState TestDepthBuffer::onCleanup() {
	_depthBuffer.shutdown();
	_meshShader.shutdown();
	_texturedFullscreenQuad.shutdown();
	_plane.shutdown();
	_shadowMapRenderShader.shutdown();
	_shadowMapShader.shutdown();
	_mesh->shutdown();
	_meshPool.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestDepthBuffer>()->startMainLoop(argc, argv);
}
