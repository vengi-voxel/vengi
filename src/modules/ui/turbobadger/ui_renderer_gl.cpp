/**
 * @file
 */

#include <stdio.h>
#include "ui_renderer_gl.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/Common.h"
#include <tb_bitmap_fragment.h>
#include <tb_system.h>

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO
static uint32_t tb_dbg_bitmap_validations = 0;
#endif

UIBitmapGL::UIBitmapGL(UIRendererGL *renderer) :
		_renderer(renderer) {
}

UIBitmapGL::~UIBitmapGL() {
	_renderer->flushBitmap(this);

	shutdown();
}

void UIBitmapGL::shutdown() {
	if (_destroy) {
		video::deleteTexture(_texture);
	}
	_destroy = false;
}

void UIBitmapGL::bind(video::TextureUnit unit) {
	video::bindTexture(unit, _textureConfig.type(), _texture);
}

bool UIBitmapGL::init(int width, int height, video::Id texture) {
	_w = width;
	_h = height;
	_texture = texture;
	_destroy = false;
	setData(nullptr);
	return true;
}

bool UIBitmapGL::init(int width, int height, uint32_t *data) {
	core_assert(width == TBGetNearestPowerOfTwo(width));
	core_assert(height == TBGetNearestPowerOfTwo(height));

	_w = width;
	_h = height;

	_destroy = true;

	_texture = video::genTexture();
	_textureConfig.format(video::TextureFormat::RGBA);
	video::bindTexture(video::TextureUnit::Upload, _textureConfig.type(), _texture);
	video::setupTexture(_textureConfig);
	setData(data);

	return true;
}

void UIBitmapGL::setData(uint32_t *data) {
	_renderer->flushBitmap(this);
	video::bindTexture(video::TextureUnit::Upload, _textureConfig.type(), _texture);
	if (data != nullptr) {
		video::uploadTexture(_textureConfig.type(), _textureConfig.format(), _w, _h, (const uint8_t*)data, 0);
	}
	TB_IF_DEBUG_SETTING(RENDER_BATCHES, tb_dbg_bitmap_validations++);
}

UIRendererGL::UIRendererGL() :
		_white(this), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

void UIRendererGL::shutdown() {
	_white.shutdown();
	_shader.shutdown();
	_vbo.shutdown();
}

void UIRendererGL::onWindowResize(const glm::ivec2& pixelDimensions, const glm::ivec2& screenDimensions) {
	_camera.init(glm::ivec2(0), pixelDimensions, screenDimensions);
	_camera.update(0L);
	video::ScopedShader scoped(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
}

bool UIRendererGL::init(const glm::ivec2& pixelDimensions, const glm::ivec2& screenDimensions) {
	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return false;
	}

	_bufferIndex = _vbo.create();
	if (_bufferIndex < 0) {
		Log::error("Failed to create ui vbo");
		return false;
	}

	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);
	_camera.init(glm::ivec2(0), pixelDimensions, screenDimensions);
	_camera.update(0L);

	_vbo.addAttribute(_shader.getColorAttribute(_bufferIndex, &Vertex::r, true));
	_vbo.addAttribute(_shader.getTexcoordAttribute(_bufferIndex, &Vertex::u));
	_vbo.addAttribute(_shader.getPosAttribute(_bufferIndex, &Vertex::x));

	uint32_t data = 0xffffffff;
	_white.init(1, 1, &data);

	return true;
}

void UIRendererGL::beginPaint(int pixelWidth, int pixelHeight) {
#ifdef TB_RUNTIME_DEBUG_INFO
	tb_dbg_bitmap_validations = 0;
#endif

	TBRendererBatcher::beginPaint(pixelWidth, pixelHeight);

	_shader.activate();
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
	_shader.setTexture(video::TextureUnit::Zero);

	video::viewport(0, 0, pixelWidth, pixelHeight);
	video::scissor(0, 0, pixelWidth, pixelHeight);

	video::enable(video::State::Blend);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
}

void UIRendererGL::endPaint() {
	TBRendererBatcher::endPaint();
	_shader.deactivate();

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES)) {
		Log::debug("Frame caused %d bitmap validations.", tb_dbg_bitmap_validations);
	}
#endif
}

void UIRendererGL::bindBitmap(TBBitmap *bitmap) {
	if (bitmap == nullptr) {
		_white.bind();
		return;
	}
	static_cast<UIBitmapGL*>(bitmap)->bind();
}

TBBitmap *UIRendererGL::createBitmap(int width, int height, uint32_t *data) {
	UIBitmapGL *bitmap = new UIBitmapGL(this);
	if (!bitmap->init(width, height, data)) {
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

void UIRendererGL::renderBatch(Batch *batch) {
	bindBitmap(batch->bitmap);
	core_assert_always(_vbo.update(_bufferIndex, batch->vertex, sizeof(Vertex) * batch->vertex_count));

	core_assert_always(_vbo.bind());
	video::drawArrays(video::Primitive::Triangles, _vbo.elements(_bufferIndex, 1, sizeof(Vertex)));
	_vbo.unbind();
}

void UIRendererGL::setClipRect(const TBRect &rect) {
	video::scissor(m_clip_rect.x, m_clip_rect.y, m_clip_rect.w, m_clip_rect.h);
}

}
