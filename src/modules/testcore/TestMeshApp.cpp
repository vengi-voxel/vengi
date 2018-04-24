/*
 * @file
 */

#include "TestMeshApp.h"
#include "core/command/Command.h"
#include "core/Trace.h"
#include "core/GameConfig.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedViewPort.h"
#include "io/Filesystem.h"

#define MaxDepthBufferUniformName "u_cascades"

TestMeshApp::TestMeshApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _colorShader(shader::ColorShader::getInstance()) {
	setCameraMotion(true);
	setRenderPlane(false);
	_fogColor = core::Color::LightBlue;
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

	_meshName = core::Var::get("mesh", "chr_skelett2_bake");
	_animationIndex = core::Var::get("animation", "0");
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_shadowMapShow = core::Var::get(cfg::ClientShadowMapShow, "false");
	_debugShadow = core::Var::getSafe(cfg::ClientDebugShadow);
	_debugShadowCascade = core::Var::getSafe(cfg::ClientDebugShadowMapCascade);

	return state;
}

core::AppState TestMeshApp::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_shadowRangeZ = _camera.farPlane() * 3.0f;

	if (!_shadow.init()) {
		Log::error("Failed to init shadow object");
		return core::AppState::InitFailure;
	}

	_camera.setType(video::CameraType::FirstPerson);
	_camera.setMode(video::CameraMode::Perspective);
	_camera.setPosition(glm::vec3(0.0f, 10.0f, 150.0f));
	_camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
	_camera.setTargetDistance(50.0f);
	_camera.setRotationType(video::CameraRotationType::Target);

	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return core::AppState::InitFailure;
	}
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
		return core::AppState::InitFailure;
	}
	if (!_meshShader.setup()) {
		Log::error("Failed to init mesh shader");
		return core::AppState::InitFailure;
	}
	if (!_colorShader.setup()) {
		Log::error("Failed to init color shader");
		return core::AppState::InitFailure;
	}

	_meshPool.init();

	const std::string& mesh = _meshName->strVal();
	_mesh = _meshPool.getMesh(mesh);
	if (!_mesh->isLoading()) {
		Log::error("Failed to load the mesh %s", mesh.c_str());
		return core::AppState::InitFailure;
	}
	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, maxDepthBuffers)) {
		Log::error("Failed to init the depthbuffer");
		return core::AppState::InitFailure;
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

void TestMeshApp::onRenderUI() {
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	static bool showMeshDetails = true;
	if (ImGui::Begin("Mesh details", &showMeshDetails)) {
		ImGui::Text("Mesh %s", _mesh->filename().c_str());
		ImGui::Text("%i vertices", (int)_mesh->vertices().size());
		ImGui::Text("%i indices", (int)_mesh->indices().size());
		ImGui::Text("%i bones", (int)_mesh->bones());
		ImGui::Text("%i animations", (int)_mesh->animations());
		ImGui::End();
	}

	static bool showInfo = true;
	ImGui::SetNextWindowPos(ImVec2(300, 10));
	if (ImGui::Begin("Info", &showInfo)) {
		Super::onRenderUI();
		ImGui::End();
	}

	ImGui::SetNextWindowPos(ImVec2(10, 150));
	static bool showOptions = true;
	if (ImGui::Begin("Options", &showOptions)) {
		ImGui::CheckboxVar("Fog", cfg::ClientFog);
		ImGui::CheckboxVar("Shadow map", _shadowMap);
		ImGui::CheckboxVar("Show shadow map", _shadowMapShow);
		ImGui::CheckboxVar("Shadow map debug", _debugShadow);
		ImGui::CheckboxVar("Show shadow cascades", _debugShadowCascade);
		static const char* items[] = { "Disable", "First", "Second", "Third", "Fourth" };
		ImGui::Combo("Bone weight", &_boneInfluence, items, IM_ARRAYSIZE(items));
		ImGui::Checkbox("Render mesh", &_renderMesh);
		ImGui::Checkbox("Render normals", &_renderNormals);
		ImGui::Checkbox("Render bones", &_renderBones);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Leaf bones are not rendered");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		if (ImGui::InputFloat3("Camera omega", glm::value_ptr(_omega))) {
			_camera.setOmega(_omega);
		}
		ImGui::InputFloat("Shadow bias", &_shadowBias, 0.001f, 0.01f);
		ImGui::InputFloat("Shadow bias slope", &_shadowBiasSlope, 0.01f, 0.1f);
		ImGui::InputFloat("Shadow range", &_shadowRangeZ, 0.01f, 0.1f);
		ImGui::InputFloat("Fog range", &_fogRange, 0.01f, 0.1f);
		if (_mesh->animations() > 1 && ImGui::InputVarInt("Animation index", _animationIndex, 1, 1)) {
			_animationIndex->setVal(_mesh->currentAnimation());
		}
		ImGui::InputVarString("Mesh", _meshName);
		if (_meshName->isDirty()) {
			const video::MeshPtr& meshPtr = _meshPool.getMesh(_meshName->strVal());
			if (meshPtr->isLoading()) {
				_mesh->shutdown();
				_mesh = meshPtr;
			} else {
				Log::warn("Failed to load mesh: %s", _meshName->strVal().c_str());
			}
			_meshName->markClean();
		}
		ImGui::InputFloat3("Position", glm::value_ptr(_position));
		ImGui::ColorEdit3("Diffuse color", glm::value_ptr(_diffuseColor));
		ImGui::ColorEdit3("Ambient color", glm::value_ptr(_ambientColor));
		ImGui::ColorEdit4("Fog color", glm::value_ptr(_fogColor));
		ImGui::ColorEdit4("Clear color", glm::value_ptr(_clearColor));
		ImGui::End();
	}
}

void TestMeshApp::doRender() {
	core_trace_scoped(TestMeshAppDoRender);
	const uint8_t animationIndex = _animationIndex->intVal();
	const float timeInSeconds = lifetimeInSecondsf();

	const bool oldDepth = video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	const bool oldCullFace = video::enable(video::State::CullFace);
	const bool oldDepthMask = video::enable(video::State::DepthMask);

	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

	_model = glm::translate(glm::mat4(1.0f), _position);
	const int maxDepthBuffers = _meshShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(_camera, true, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();

	{
		core_trace_scoped(TestMeshAppDoRenderShadows);
		const bool oldBlend = video::disable(video::State::Blend);
		// put shadow acne into the dark
		const bool cullFaceChanged = video::cullFace(video::Face::Front);
		const glm::vec2 offset(_shadowBiasSlope, (_shadowBias / _shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind();
		video::ScopedShader scoped(_shadowMapShader);
		if (_mesh->initMesh(_shadowMapShader, timeInSeconds, animationIndex)) {
			_shadowMapShader.recordUsedUniforms(true);
			_shadowMapShader.clearUsedUniforms();
			_shadowMapShader.setModel(_model);
			for (int i = 0; i < maxDepthBuffers; ++i) {
				_depthBuffer.bindTexture(i);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				if (_renderPlane) {
					renderPlane(&_shadowMapShader);
				}
				if (_renderMesh) {
					// TODO: why does only the plane appear in the depth map?
					_mesh->render();
				}
			}
		} else {
			_shadowMapShader.recordUsedUniforms(false);
		}
		_depthBuffer.unbind();
		if (cullFaceChanged) {
			video::cullFace(video::Face::Back);
		}
		if (oldBlend) {
			video::enable(video::State::Blend);
		}
	}

	bool meshInitialized = true;
	{
		if (_renderPlane) {
			renderPlane();
		}

		if (_renderMesh) {
			video::ScopedShader scoped(_meshShader);
			_meshShader.clearUsedUniforms();
			_meshShader.recordUsedUniforms(true);
			meshInitialized = _mesh->initMesh(_meshShader, timeInSeconds, animationIndex);
			if (meshInitialized) {
				_meshShader.setViewprojection(_camera.viewProjectionMatrix());
				_meshShader.setFogrange(_fogRange);
				_meshShader.setViewdistance(_camera.farPlane());
				_meshShader.setModel(_model);
				_meshShader.setTexture(video::TextureUnit::Zero);
				_meshShader.setDiffuseColor(_diffuseColor);
				_meshShader.setAmbientColor(_ambientColor);
				_meshShader.setShadowmap(video::TextureUnit::One);
				_meshShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
				_meshShader.setFogcolor(_fogColor);
				_meshShader.setCascades(cascades);
				_meshShader.setDistances(distances);
				_meshShader.setLightdir(_shadow.sunDirection());
				_meshShader.setBoneinfluence(_boneInfluence - 1);
				video::bindTexture(video::TextureUnit::One, _depthBuffer);
				const video::ScopedPolygonMode scopedPolygonMode(_camera.polygonMode());
				_mesh->render();
			} else {
				_meshShader.recordUsedUniforms(false);
			}
		}
	}
	if (meshInitialized) {
		if (_renderNormals || _renderBones) {
			video::ScopedShader scoped(_colorShader);
			_colorShader.recordUsedUniforms(true);
			_colorShader.clearUsedUniforms();
			_colorShader.setViewprojection(_camera.viewProjectionMatrix());
			_colorShader.setModel(_model);
			if (_renderNormals) {
				core_trace_scoped(TestMeshAppDoNormals);
				_mesh->renderNormals(_colorShader);
			}
			if (_renderBones) {
				core_trace_scoped(TestMeshAppDoBones);
				_mesh->renderBones(_colorShader);
			}
		}
	}

	if (_shadowMapShow->boolVal()) {
		core_trace_scoped(TestMeshAppDoShowShadowMap);
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
		video::ScopedVertexBuffer scopedBuf(_shadowMapDebugBuffer);

		// configure shadow map texture
		video::bindTexture(video::TextureUnit::Zero, _depthBuffer);
		video::setupDepthCompareTexture(_depthBuffer.textureType(), video::CompareFunc::Less, video::TextureCompareMode::None);

		// render shadow maps
		for (int i = 0; i < maxDepthBuffers; ++i) {
			const int halfWidth = (int) (width / 4.0f);
			const int halfHeight = (int) (height / 4.0f);
			video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
			_shadowMapRenderShader.setCascade(i);
			video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));
		}

		// restore texture
		video::setupDepthCompareTexture(_depthBuffer.textureType(), video::CompareFunc::Less, video::TextureCompareMode::RefToTexture);
	}

	if (!oldDepth) {
		video::disable(video::State::DepthTest);
	}
	if (!oldCullFace) {
		video::disable(video::State::CullFace);
	}
	if (!oldDepthMask) {
		video::disable(video::State::DepthMask);
	}
}

void TestMeshApp::renderPlane(video::Shader* shader) {
	_plane.render(_camera, _model, shader);
}

core::AppState TestMeshApp::onCleanup() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_depthBuffer.shutdown();
	_meshShader.shutdown();
	_colorShader.shutdown();
	_shadowMapShader.shutdown();
	_shadow.shutdown();
	if (_mesh) {
		_mesh->shutdown();
	}
	_meshPool.shutdown();
	return Super::onCleanup();
}
