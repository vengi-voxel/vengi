#include "TestMeshApp.h"
#include "core/command/Command.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedViewPort.h"
#include "io/Filesystem.h"

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
			Log::error("Usage: loadmesh <meshname>");
			return;
		}
		const std::string& mesh = args[0];
		Log::info("Trying to load mesh %s", mesh.c_str());
		const video::MeshPtr& meshPtr = _meshPool.getMesh(mesh);
		if (meshPtr->isLoading()) {
			_mesh->shutdown();
			_mesh = meshPtr;
		} else {
			Log::warn("Failed to load mesh: %s", mesh.c_str());
		}
	}).setHelp("Load a mesh from the pool. The name is without extension and the file must be in the mesh/ dir.");

	core::Var::get("mesh", "chr_skelett2_bake");
	core::Var::get("animation", "0");
	_shadowMapShow = core::Var::get(cfg::ClientShadowMapShow, "false");

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
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
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

	const std::string& mesh = core::Var::getSafe("mesh")->strVal();
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

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	return state;
}

void TestMeshApp::doRender() {
	const uint8_t animationIndex = core::Var::getSafe("animation")->intVal();
	const long timeInSeconds = lifetimeInSeconds();

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(_camera, true, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();

	{
		video::disable(video::State::Blend);
		// put shadow acne into the dark
		video::cullFace(video::Face::Front);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = _camera.farPlane() * 3.0f;
		const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind();
		video::ScopedShader scoped(_shadowMapShader);
		if (_mesh->initMesh(_shadowMapShader, timeInSeconds, animationIndex)) {
			_shadowMapShader.recordUsedUniforms(true);
			_shadowMapShader.clearUsedUniforms();
			_shadowMapShader.setModel(glm::mat4(1.0f));
			for (int i = 0; i < maxDepthBuffers; ++i) {
				_depthBuffer.bindTexture(i);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				renderPlane();
				_mesh->render();
			}
		} else {
			_shadowMapShader.recordUsedUniforms(false);
		}
		_depthBuffer.unbind();
		video::cullFace(video::Face::Back);
		video::enable(video::State::Blend);
	}

	bool meshInitialized = false;
	{
		video::clearColor(glm::vec4(0.8, 0.8f, 0.8f, 1.0f));
		video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

		renderPlane();

		video::ScopedShader scoped(_meshShader);
		_meshShader.clearUsedUniforms();
		_meshShader.recordUsedUniforms(true);
		meshInitialized = _mesh->initMesh(_meshShader, timeInSeconds, animationIndex);
		if (meshInitialized) {
			_meshShader.setViewprojection(_camera.viewProjectionMatrix());
			_meshShader.setFogrange(250.0f);
			_meshShader.setViewdistance(_camera.farPlane());
			_meshShader.setModel(glm::mat4(1.0f));
			_meshShader.setTexture(video::TextureUnit::Zero);
			_meshShader.setDiffuseColor(_diffuseColor);
			_meshShader.setAmbientColor(_ambientColor);
			_meshShader.setShadowmap(video::TextureUnit::One);
			_meshShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
			_meshShader.setFogcolor(core::Color::LightBlue);
			_meshShader.setCascades(cascades);
			_meshShader.setDistances(distances);
			_meshShader.setLightdir(_shadow.sunDirection());
			video::bindTexture(video::TextureUnit::One, _depthBuffer);
			const video::ScopedPolygonMode scopedPolygonMode(_camera.polygonMode());
			_mesh->render();
		} else {
			_meshShader.recordUsedUniforms(false);
		}
	}
	if (meshInitialized) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.recordUsedUniforms(true);
		_colorShader.clearUsedUniforms();
		_colorShader.setViewprojection(_camera.viewProjectionMatrix());
		_colorShader.setModel(glm::mat4(1.0f));
		_mesh->renderNormals(_colorShader);
	}

	if (_shadowMapShow->boolVal()) {
		const int width = _camera.width();
		const int height = _camera.height();

		// activate shader
		video::ScopedShader scopedShader(_shadowMapRenderShader);
		_shadowMapRenderShader.recordUsedUniforms(true);
		_shadowMapRenderShader.clearUsedUniforms();
		_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
		_shadowMapRenderShader.setFar(_camera.farPlane());
		_shadowMapRenderShader.setNear(_camera.nearPlane());

		// bind buffers
		core_assert_always(_shadowMapDebugBuffer.bind());

		// configure shadow map texture
		video::bindTexture(video::TextureUnit::Zero, _depthBuffer);
		if (_depthBuffer.depthCompare()) {
			video::disableDepthCompareTexture(video::TextureUnit::Zero, _depthBuffer.textureType(), _depthBuffer.texture());
		}

		// render shadow maps
		for (int i = 0; i < maxDepthBuffers; ++i) {
			const int halfWidth = (int) (width / 4.0f);
			const int halfHeight = (int) (height / 4.0f);
			video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
			_shadowMapRenderShader.setCascade(i);
			video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));
		}

		// restore texture
		if (_depthBuffer.depthCompare()) {
			video::setupDepthCompareTexture(video::TextureUnit::Zero, _depthBuffer.textureType(), _depthBuffer.texture());
		}

		// unbind buffer
		_shadowMapDebugBuffer.unbind();
	}
}

void TestMeshApp::renderPlane() {
	_plane.render(_camera);
}

core::AppState TestMeshApp::onCleanup() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_depthBuffer.shutdown();
	_meshShader.shutdown();
	_colorShader.shutdown();
	_shadowMapShader.shutdown();
	_mesh->shutdown();
	_meshPool.shutdown();
	return Super::onCleanup();
}
