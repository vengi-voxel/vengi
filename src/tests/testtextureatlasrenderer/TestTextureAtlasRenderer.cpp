/**
 * @file
 */
#include "TestTextureAtlasRenderer.h"
#include "testcore/TestAppMain.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/MeshCache.h"
#include "video/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

TestTextureAtlasRenderer::TestTextureAtlasRenderer(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider),
		_meshRenderer(std::make_shared<voxelformat::MeshCache>()) {
	init(ORGANISATION, "testtextureatlasrenderer");
}

core::AppState TestTextureAtlasRenderer::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}
	if (!_meshRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::InitFailure;
	}
	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return core::AppState::InitFailure;
	}

	const glm::ivec2& wd = windowDimension();
	_bufIdx = _vbo.create(nullptr, sizeof(_vertices));
	if (_bufIdx == -1) {
		Log::error("Failed to create vertex buffer");
		return core::AppState::InitFailure;
	}

	_vbo.addAttribute(_textureShader.getColorAttribute(_bufIdx, &Vertex::r, true));
	_vbo.addAttribute(_textureShader.getTexcoordAttribute(_bufIdx, &Vertex::u));
	_vbo.addAttribute(_textureShader.getPosAttribute(_bufIdx, &Vertex::x));

	if (!_atlasRenderer.init()) {
		Log::error("Failed to initialize the atlas renderer");
		return core::AppState::InitFailure;
	}
	_modelIndex = _meshRenderer.addMesh("assets/north-dir");
	if (_modelIndex == -1) {
		Log::error("Failed to load model");
		return core::AppState::InitFailure;
	}

	const glm::vec2 vecs[lengthof(_vertices)] = {
		// left bottom, right bottom, right top
		glm::vec2(0.0f, wd.y), glm::vec2(wd.x, wd.y), glm::vec2(wd.x, 0.0f),
		// left bottom, right top, left top
		glm::vec2(0.0f, wd.y), glm::vec2(wd.x, 0.0f), glm::vec2(0.0f, 0.0f)
	};
	for (int i = 0; i < lengthof(_vertices); ++i) {
		_vertices[i].pos = vecs[i];
		_vertices[i].color = 0xFFFFFFFFU;
	}

	updateModelMatrix();

	video::clearColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

	return state;
}

void TestTextureAtlasRenderer::updateModelMatrix() {
	constexpr glm::mat4 translate(1.0f);
	constexpr double tau = glm::two_pi<double>();
	const double orientation = glm::mod(_omegaY * _nowSeconds, tau);
	const glm::mat4 rot = glm::rotate(translate, (float)orientation, glm::up);
	_modelMatrix = glm::scale(rot, glm::vec3(_scale));
}

core::AppState TestTextureAtlasRenderer::onCleanup() {
	_meshRenderer.shutdown();
	_atlasRenderer.shutdown();
	_textureShader.shutdown();
	_vbo.shutdown();
	return Super::onCleanup();
}

void TestTextureAtlasRenderer::onRenderUI() {
	Super::onRenderUI();
	ImGui::InputFloat("Scale", &_scale, 0.01f, 0.1f);
	ImGui::InputFloat("omega", &_omegaY, 0.1f, 1.0f);
	constexpr double tau = glm::two_pi<double>();
	const double orientation = glm::mod(_omegaY * _nowSeconds, tau);
	ImGui::Text("orientation: %f", orientation);
	ImGui::Text("seconds: %.2f", _nowSeconds);
}

void TestTextureAtlasRenderer::doRender() {
	updateModelMatrix();

	// render to texture
	const video::TextureAtlasData& data = _atlasRenderer.beginRender(0, camera().frameBufferWidth(), camera().frameBufferHeight());
	_meshRenderer.setModelMatrix(_modelIndex, _modelMatrix);
	_meshRenderer.render(_modelIndex, camera());
	_atlasRenderer.endRender();

	// update uv coordinates in the vertex buffer
	// this should only be needed once in the test - but theoretically the atlas renderer
	// could assign a different location in the framebuffer texture for the given id.
	_vertices[0].uv = glm::vec2(data.sx, data.sy);	// left bottom
	_vertices[1].uv = glm::vec2(data.tx, data.sy);	// right bottom
	_vertices[2].uv = glm::vec2(data.tx, data.ty);	// right top
	_vertices[3].uv = glm::vec2(data.sx, data.sy);	// left bottom
	_vertices[4].uv = glm::vec2(data.tx, data.ty);	// right top
	_vertices[5].uv = glm::vec2(data.sx, data.ty);	// left top
	core_assert_always(_vbo.update(_bufIdx, _vertices, sizeof(_vertices)));

	// render texture to screen
	video::ScopedShader scoped(_textureShader);
	if (_textureShader.isDirty()) {
		const glm::ivec2& wd = windowDimension();
		_textureShader.setModel(glm::mat4(1.0f));
		_textureShader.setTexture(video::TextureUnit::Zero);
		_textureShader.setViewprojection(glm::ortho(0.0f, (float)wd.x, (float)wd.y, 0.0f, -1.0f, 1.0f));
		_textureShader.markClean();
	}
	video::ScopedBuffer scopedBuf(_vbo);
	video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, data.handle);
	video::drawArrays(video::Primitive::Triangles, lengthof(_vertices));
}

TEST_APP(TestTextureAtlasRenderer)
