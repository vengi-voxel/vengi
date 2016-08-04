#include "TestDepthBuffer.h"
#include "video/ScopedViewPort.h"

TestDepthBuffer::TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestDepthBuffer::onInit() {
	core::AppState state = Super::onInit();

	_sunLight.setPos(glm::vec3(20.0f, 50.0f, -20.0));
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 150.0f));
	_camera.lookAt(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setOmega(glm::vec3(0.0f, 0.1f, 0.0f));
	_camera.setTarget(glm::vec3(0.0f, 50.0f, 0.0f));
	_camera.setTargetDistance(150.0f);
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

	const std::string mesh = "mesh/chr_skelett2_bake.FBX";
	if (!_mesh.loadMesh(mesh)) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}
	if (!_depthBuffer.init(_dimension)) {
		Log::error("Failed to init the depthbuffer");
		return core::AppState::Cleanup;
	}

	return state;
}

void TestDepthBuffer::doRender() {
	_sunLight.update(_deltaFrame, _camera);
	{
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setLight(_sunLight.model());
		_shadowMapShader.setModel(glm::mat4());
		if (!_mesh.initMesh(_shadowMapShader)) {
			Log::error("Failed to init the mesh for the shadow map stage");
		}
		glDisable(GL_BLEND);
		glCullFace(GL_FRONT);
		_depthBuffer.bind();
		_mesh.render();
		_depthBuffer.unbind();
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
	}
	{
		glClearColor(0.8, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		video::ScopedShader scoped(_meshShader);
		_meshShader.setView(_camera.viewMatrix());
		_meshShader.setProjection(_camera.projectionMatrix());
		_meshShader.setFogrange(500.0f);
		_meshShader.setViewdistance(500.0f);
		_meshShader.setModel(glm::mat4());
		_meshShader.setLightpos(_sunLight.dir() + _camera.position());
		_meshShader.setTexture(0);

		if (!_mesh.initMesh(_meshShader)) {
			Log::error("Failed to init the mesh for the render stage");
			return;
		}
		core_assert_always(_mesh.render() > 0);
	}
	{
		video::ScopedShader scoped(_shadowMapRenderShader);
		_shadowMapRenderShader.setShadowmap(0);
		const int width = _camera.width();
		const int height = _camera.height();
		const GLsizei quadWidth = (GLsizei) (width / 3.0f);
		const GLsizei quadHeight = (GLsizei) (height / 3.0f);
		video::ScopedViewPort scopedViewport(width - quadWidth, 0, quadWidth, quadHeight);
		core_assert_always(_texturedFullscreenQuad.bind());
		glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture());
		glDrawArrays(GL_TRIANGLES, 0, _texturedFullscreenQuad.elements(0));
		_texturedFullscreenQuad.unbind();
	}
}

core::AppState TestDepthBuffer::onCleanup() {
	_depthBuffer.shutdown();
	_meshShader.shutdown();
	_texturedFullscreenQuad.shutdown();
	_shadowMapRenderShader.shutdown();
	_shadowMapShader.shutdown();
	_mesh.shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestDepthBuffer>()->startMainLoop(argc, argv);
}
