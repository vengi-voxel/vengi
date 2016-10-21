#include "TestMeshApp.h"
#include "core/Command.h"
#include "video/ScopedPolygonMode.h"

#define MaxDepthBufferUniformName "u_farplanes"

TestMeshApp::TestMeshApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider), _colorShader(shader::ColorShader::getInstance()) {
	setCameraMotion(true);
	setRenderPlane(false);
}

core::AppState TestMeshApp::onInit() {
	core::AppState state = Super::onInit();

	core::Command::registerCommand("loadmesh", [this] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: %s <meshname>", args[0].c_str());
			return;
		}
		const std::string& mesh = args[1];
		const video::MeshPtr& meshPtr = _meshPool.getMesh(mesh);
		if (meshPtr->isLoaded()) {
			_mesh->shutdown();
			_mesh = meshPtr;
		}
	});

	const glm::vec3 sunDirection(glm::left.x, glm::down.y, 0.0f);
	_sunLight.init(sunDirection, dimension());
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

	_meshPool.init();

	const std::string mesh = core::Var::get("mesh", "chr_skelett2_bake")->strVal();
	_mesh = _meshPool.getMesh(mesh);
	if (!_mesh->isLoading()) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}
	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	if (!_depthBuffer.init(_dimension, video::DepthBufferMode::RGBA, maxDepthBuffers)) {
		Log::error("Failed to init the depthbuffer");
		return core::AppState::Cleanup;
	}

	return state;
}

void TestMeshApp::doRender() {
	_sunLight.update(_deltaFrame, _camera);
	// TODO: support different animations...
	const uint8_t animationIndex = core::Var::get("animation", "0")->intVal();
	const long timeInSeconds = (_now - _initTime) / 1000.0f;
	{
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setLight(_sunLight.viewProjectionMatrix(_camera));
		_shadowMapShader.setModel(glm::mat4());
		if (_mesh->initMesh(_shadowMapShader, timeInSeconds, animationIndex)) {
			video::ScopedPolygonMode scopedPolygonMode(_camera.polygonMode());
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
		_meshShader.setLightdir(_sunLight.direction());
		_meshShader.setTexture(0);
		_meshShader.setDiffuseColor(_diffuseColor);
		_meshShader.setDepthsize(glm::vec2(_sunLight.dimension()));
		_meshShader.setLight(_sunLight.viewProjectionMatrix(_camera));
		_meshShader.setShadowmap1(1);
		_meshShader.setShadowmap2(2);

		meshInitialized = _mesh->initMesh(_meshShader, timeInSeconds, animationIndex);
		if (meshInitialized) {
			const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
			for (int i = 0; i < maxDepthBuffers; ++i) {
				glActiveTexture(GL_TEXTURE1 + i);
				glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture(i));
			}

			video::ScopedPolygonMode scopedPolygonMode(_camera.polygonMode());
			core_assert_always(_mesh->render() > 0);
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
