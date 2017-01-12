#include "TestMeshApp.h"
#include "core/command/Command.h"

#define MaxDepthBufferUniformName "u_cascades"

TestMeshApp::TestMeshApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider), _colorShader(shader::ColorShader::getInstance()) {
	setCameraMotion(true);
	setRenderPlane(false);
}

core::AppState TestMeshApp::onConstruct() {
	core::AppState state = Super::onConstruct();

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
	}).setHelp("Load a mesh from the pool. The name is without extension and the file must be in the mesh/ dir.");

	core::Var::get(cfg::ClientShadowMapSize, "512");
	core::Var::get("mesh", "chr_skelett2_bake");
	core::Var::get("animation", "0");

	return state;
}

core::AppState TestMeshApp::onInit() {
	core::AppState state = Super::onInit();

	if (!_shadow.init()) {
		Log::error("Failed to init shadow object");
		return core::AppState::Cleanup;
	}

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

	const std::string mesh = core::Var::getSafe("mesh")->strVal();
	_mesh = _meshPool.getMesh(mesh);
	if (!_mesh->isLoading()) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::Cleanup;
	}
	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		Log::error("Failed to init the depthbuffer");
		return core::AppState::Cleanup;
	}

	return state;
}

void TestMeshApp::doRender() {
	const uint8_t animationIndex = core::Var::getSafe("animation")->intVal();
	const long timeInSeconds = (_now - _initTime) / 1000.0f;

	video::enable(video::State::DepthTest);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);
	// Cull triangles whose normal is not towards the camera
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(_camera, true, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();

	{
		video::disable(video::State::Blend);
		// put shadow acne into the dark
		glCullFace(GL_FRONT);
		video::enable(video::State::PolygonOffsetFill);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = _camera.farPlane() * 3.0f;
		glPolygonOffset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));

		_depthBuffer.bind();
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setModel(glm::mat4());
		if (_mesh->initMesh(_shadowMapShader, timeInSeconds, animationIndex)) {
			for (int i = 0; i < maxDepthBuffers; ++i) {
				_depthBuffer.bindTexture(i);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				renderPlane();
				_mesh->render();
			}
		}
		_depthBuffer.unbind();
		glCullFace(GL_BACK);
		video::enable(video::State::Blend);
		video::disable(video::State::PolygonOffsetFill);
	}

	bool meshInitialized = false;
	{
		video::clearColor(glm::vec4(0.8, 0.8f, 0.8f, 1.0f));
		video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

		renderPlane();

		video::ScopedShader scoped(_meshShader);
		//_meshShader.setLightdir(_shadow.sunDirection());
		_meshShader.setView(_camera.viewMatrix());
		_meshShader.setProjection(_camera.projectionMatrix());
		_meshShader.setViewprojection(_camera.viewProjectionMatrix());
		_meshShader.setFogrange(250.0f);
		_meshShader.setViewdistance(_camera.farPlane());
		_meshShader.setModel(glm::mat4());
		_meshShader.setTexture(video::TextureUnit::Zero);
		_meshShader.setDiffuseColor(_diffuseColor);
		_meshShader.setAmbientColor(_ambientColor);
		_meshShader.setShadowmap(video::TextureUnit::One);
		_meshShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		_meshShader.setFogcolor(core::Color::LightBlue);
		_meshShader.setCascades(cascades);
		_meshShader.setDistances(distances);
		_meshShader.setLightdir(_shadow.sunDirection());

		meshInitialized = _mesh->initMesh(_meshShader, timeInSeconds, animationIndex);
		if (meshInitialized) {
			video::bindTexture(video::TextureUnit::One, _depthBuffer);
			_mesh->render();
		}
	}
	if (meshInitialized) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.setViewprojection(_camera.viewProjectionMatrix());
		_mesh->renderNormals(_colorShader);
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
