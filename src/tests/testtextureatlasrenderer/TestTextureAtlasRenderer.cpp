/**
 * @file
 */
#include "TestTextureAtlasRenderer.h"
#include "testcore/TestAppMain.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/MeshCache.h"
#include "video/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

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
	_modelIndex = _meshRenderer.addMesh("assets/north-dir.vox");
	if (_modelIndex == -1) {
		Log::error("Failed to load model");
		return core::AppState::InitFailure;
	}
	{
		video::ScopedShader scoped(_textureShader);
		_textureShader.setModel(glm::mat4(1.0f));
		_textureShader.setTexture(video::TextureUnit::Zero);
		_textureShader.setViewprojection(glm::ortho(0.0f, (float)wd.x, (float)wd.y, 0.0f, -1.0f, 1.0f));
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

	return state;
}

core::AppState TestTextureAtlasRenderer::onCleanup() {
	core::AppState state = Super::onCleanup();
	_meshRenderer.shutdown();
	_atlasRenderer.shutdown();
	_textureShader.shutdown();
	_vbo.shutdown();
	return state;
}

void TestTextureAtlasRenderer::doRender() {
	// render to texture
	const video::TextureAtlasData& data = _atlasRenderer.beginRender(0, camera().frameBufferWidth(), camera().frameBufferHeight());
	_meshRenderer.render(_modelIndex, camera());
	_atlasRenderer.endRender();

	// update uv coordinates in the vertex buffer
	// this should only be needed once in the test - but theoretically the atlas renderer
	// could assign a different location in the framebuffer texture for the given id.
	const float x = (float)data.x / (float)data.texWidth;
	const float y = (float)data.y / (float)data.texHeight;
	const float w = (float)data.w / (float)data.texWidth;
	const float h = (float)data.h / (float)data.texHeight;
	_vertices[0].uv = glm::vec2(x, y);			// left bottom
	_vertices[1].uv = glm::vec2(x + w, y);		// right bottom
	_vertices[2].uv = glm::vec2(x + w, y + h);	// right top
	_vertices[3].uv = glm::vec2(x, y);			// left bottom
	_vertices[4].uv = glm::vec2(x + w, y + h);	// right top
	_vertices[5].uv = glm::vec2(x, y + h);		// left top
	core_assert_always(_vbo.update(_bufIdx, _vertices, sizeof(_vertices)));

	// render texture to screen
	video::ScopedShader scoped(_textureShader);
	video::ScopedBuffer scopedBuf(_vbo);
	video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, data.handle);
	video::drawArrays(video::Primitive::Triangles, lengthof(_vertices));
}

TEST_APP(TestTextureAtlasRenderer)
