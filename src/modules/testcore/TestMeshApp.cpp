#include "TestMeshApp.h"

static constexpr uint32_t FL_RENDER_LINES = 1 << 0;

TestMeshApp::TestMeshApp(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
	setRenderPlane(false);
}

core::AppState TestMeshApp::onInit() {
	core::AppState state = Super::onInit();

	_sunLight.init(glm::vec3(20.0f, 50.0f, -20.0), dimension());
	_camera.setPosition(glm::vec3(0.0f, 10.0f, 150.0f));
	_camera.setOmega(glm::vec3(0.0f, 0.1f, 0.0f));
	_camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	_camera.setTargetDistance(50.0f);
	_camera.setRotationType(video::CameraRotationType::Target);

	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return core::AppState::Cleanup;
	}
	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::Cleanup;
	}
	if (!_colorShader.setup()) {
		Log::error("Failed to init color shader");
		return core::AppState::Cleanup;
	}

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

void TestMeshApp::doRender() {
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
			core_assert_always(_mesh->render() > 0);
			_depthBuffer.unbind();
			glCullFace(GL_BACK);
			glEnable(GL_BLEND);
		}
	}
	bool meshInitialized = false;
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
		_meshShader.setDiffuseColor(_diffuseColor);
		_meshShader.setScreensize(glm::vec2(_camera.dimension()));
		_meshShader.setLight(_sunLight.modelViewProjectionMatrix());
		_meshShader.setShadowmap(1);

		meshInitialized = _mesh->initMesh(_meshShader, timeInSeconds, animationIndex);
		if (meshInitialized) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture());

			const bool renderLines = (_flags & FL_RENDER_LINES) != 0 || (SDL_GetModState() & KMOD_SHIFT) != 0;

			GLint polygonMode = GL_FILL;
			if (renderLines) {
				glGetIntegerv(GL_POLYGON_MODE, &polygonMode);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}

			core_assert_always(_mesh->render() > 0);

			if (renderLines) {
				glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
			}

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);
		}
	}
	if (meshInitialized) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.setView(_camera.viewMatrix());
		_colorShader.setProjection(_camera.projectionMatrix());
		core_assert_always(_mesh->renderNormals(_colorShader) > 0);
	}
}

void TestMeshApp::renderPlane() {
	_plane.render(_camera);
}

core::AppState TestMeshApp::onCleanup() {
	_depthBuffer.shutdown();
	_meshShader.shutdown();
	_colorShader.shutdown();
	_shadowMapShader.shutdown();
	_mesh->shutdown();
	_meshPool.shutdown();
	return Super::onCleanup();
}

bool TestMeshApp::onKeyPress(int32_t key, int16_t modifier) {
	const bool retVal = Super::onKeyPress(key, modifier);
	if (key == SDLK_KP_SPACE || key == SDLK_SPACE) {
		_flags |= FL_RENDER_LINES;
	}
	return retVal;
}

bool TestMeshApp::onKeyRelease(int32_t key) {
	const bool retVal = Super::onKeyRelease(key);
	if (key == SDLK_KP_SPACE || key == SDLK_SPACE) {
		_flags &= ~FL_RENDER_LINES;
	}
	return retVal;
}
