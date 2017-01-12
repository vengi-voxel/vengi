/**
 * @file
 */

#include <stdio.h>
#include "ui_renderer_gl.h"
#include "core/GLM.h"
#include "core/Common.h"
#include <tb_bitmap_fragment.h>
#include <tb_system.h>

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO
static uint32 dbg_bitmap_validations = 0;
#endif

GLuint UIBitmapGL::g_current_texture = (GLuint) -1;

UIBitmapGL::UIBitmapGL(UIRendererGL *renderer) :
		_renderer(renderer), _w(0), _h(0), _texture(0) {
}

UIBitmapGL::~UIBitmapGL() {
	_renderer->FlushBitmap(this);

	if (_destroy && _texture != 0u) {
		glDeleteTextures(1, &_texture);
		_texture = 0u;
	}
}

void UIBitmapGL::bind() {
	if (_texture != g_current_texture) {
		g_current_texture = _texture;
		glActiveTexture(std::enum_value(video::TextureUnit::Zero));
		glBindTexture(GL_TEXTURE_2D, _texture);
	}
}

bool UIBitmapGL::Init(int width, int height, GLuint texture) {
	_w = width;
	_h = height;
	_texture = texture;
	_destroy = false;
	SetData(nullptr);
	return true;
}

bool UIBitmapGL::Init(int width, int height, uint32 *data) {
	core_assert(width == TBGetNearestPowerOfTwo(width));
	core_assert(height == TBGetNearestPowerOfTwo(height));

	_w = width;
	_h = height;

	_destroy = true;

	glGenTextures(1, &_texture);
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GL_checkError();

	SetData(data);

	return true;
}

void UIBitmapGL::SetData(uint32 *data) {
	_renderer->FlushBitmap(this);
	bind();
	if (data != nullptr) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _w, _h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	TB_IF_DEBUG_SETTING(RENDER_BATCHES, dbg_bitmap_validations++);
}

UIRendererGL::UIRendererGL() :
		_white(this), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

void UIRendererGL::shutdown() {
	_shader.shutdown();
	_vbo.shutdown();
}

void UIRendererGL::onWindowResize(const glm::ivec2& dimensions) {
	_camera.init(glm::ivec2(0), dimensions);
	_camera.update(0L);
	video::ScopedShader scoped(_shader);
	_shader.setProjection(_camera.projectionMatrix());
}

bool UIRendererGL::init(const glm::ivec2& dimensions) {
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
	_camera.init(glm::ivec2(0), dimensions);
	_camera.update(0L);

	video::VertexBuffer::Attribute attributeColor;
	attributeColor.bufferIndex = _bufferIndex;
	attributeColor.index = _shader.getLocationColor();
	attributeColor.size = _shader.getComponentsColor();
	attributeColor.stride = sizeof(Vertex);
	attributeColor.offset = offsetof(Vertex, col);
	attributeColor.type = GL_UNSIGNED_BYTE;
	attributeColor.normalized = true; // TODO: why true?
	_vbo.addAttribute(attributeColor);

	video::VertexBuffer::Attribute attributeTexCoord;
	attributeTexCoord.bufferIndex = _bufferIndex;
	attributeTexCoord.index = _shader.getLocationTexcoord();
	attributeTexCoord.size = _shader.getComponentsTexcoord();
	attributeTexCoord.stride = sizeof(Vertex);
	attributeTexCoord.offset = offsetof(Vertex, u);
	_vbo.addAttribute(attributeTexCoord);

	video::VertexBuffer::Attribute attributePosition;
	attributePosition.bufferIndex = _bufferIndex;
	attributePosition.index = _shader.getLocationPos();
	attributePosition.size = _shader.getComponentsPos();
	attributePosition.stride = sizeof(Vertex);
	attributePosition.offset = offsetof(Vertex, x);
	_vbo.addAttribute(attributePosition);

	uint32_t data = 0xffffffff;
	_white.Init(1, 1, &data);

	return true;
}

void UIRendererGL::BeginPaint(int, int) {
#ifdef TB_RUNTIME_DEBUG_INFO
	dbg_bitmap_validations = 0;
#endif

	const int renderTargetW = _camera.width();
	const int renderTargetH = _camera.height();

	TBRendererBatcher::BeginPaint(renderTargetW, renderTargetH);

	_shader.activate();
	_shader.setProjection(_camera.projectionMatrix());
	_shader.setTexture(video::TextureUnit::Zero);

	UIBitmapGL::g_current_texture = (GLuint) -1;

	glViewport(0, 0, renderTargetW, renderTargetH);
	glScissor(0, 0, renderTargetW, renderTargetH);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_checkError();

	core_assert_always(_vbo.bind());
}

void UIRendererGL::EndPaint() {
	TBRendererBatcher::EndPaint();
	_vbo.unbind();
	_shader.deactivate();

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES))
		TBDebugPrint("Frame caused %d bitmap validations.\n", dbg_bitmap_validations);
#endif
}

void UIRendererGL::bindBitmap(TBBitmap *bitmap) {
	if (bitmap == nullptr) {
		_white.bind();
		return;
	}
	static_cast<UIBitmapGL*>(bitmap)->bind();
}

TBBitmap *UIRendererGL::CreateBitmap(int width, int height, uint32 *data) {
	UIBitmapGL *bitmap = new UIBitmapGL(this);
	if (!bitmap->Init(width, height, data)) {
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

void UIRendererGL::RenderBatch(Batch *batch) {
	bindBitmap(batch->bitmap);
	core_assert_always(_vbo.update(_bufferIndex, batch->vertex, sizeof(Vertex) * batch->vertex_count));
	glDrawArrays(GL_TRIANGLES, 0, _vbo.elements(_bufferIndex, _shader.getComponentsPos()));
	GL_checkError();
}

void UIRendererGL::SetClipRect(const TBRect &rect) {
	glScissor(m_clip_rect.x, m_screen_rect.h - (m_clip_rect.y + m_clip_rect.h), m_clip_rect.w, m_clip_rect.h);
	GL_checkError();
}

}
