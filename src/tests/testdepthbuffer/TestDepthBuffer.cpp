#include "TestDepthBuffer.h"
#include "video/ScopedViewPort.h"

TestDepthBuffer::TestDepthBuffer(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
}

core::AppState TestDepthBuffer::onInit() {
	core::AppState state = Super::onInit();
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

	const std::string mesh = "animal_chicken.dae";
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
	const glm::mat4 lightProjection =
			glm::scale(
				glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 1.0f)),
				glm::vec3(1.0f, 1.0f, 0.5f)
			)
			*
			glm::ortho(-75.0f, +75.0f, -75.0f, +75.0f, 1.0f, 400.0f);
	const glm::mat4 lightView = glm::lookAt(glm::vec3(50.0f, 50.0f, -50.0f), glm::vec3(0.0f), glm::up);
	const glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	const glm::vec3 lightDir = glm::vec3(glm::column(glm::inverse(lightView), 2));
	{
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setLight(lightSpaceMatrix);
		_shadowMapShader.setModel(glm::mat4());
		if (!_mesh.initMesh(_shadowMapShader)) {
			Log::error("Failed to init the mesh");
		}
		_depthBuffer.bind();
		_mesh.render();
		_depthBuffer.unbind();
	}
	{
		video::ScopedShader scoped(_meshShader);
		_meshShader.setView(_camera.viewMatrix());
		_meshShader.setProjection(_camera.projectionMatrix());
		_meshShader.setFogrange(500.0f);
		_meshShader.setViewdistance(500.0f);
		_meshShader.setModel(glm::mat4());
		_meshShader.setLightpos(lightDir + _camera.position());
		_meshShader.setTexture(0);

		if (!_mesh.initMesh(_meshShader)) {
			Log::error("Failed to init the mesh");
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
