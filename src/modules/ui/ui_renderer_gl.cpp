#include <assert.h>
#include <stdio.h>
#include "ui_renderer_gl.h"
#include <glm/gtc/matrix_transform.hpp>
#include <tb_bitmap_fragment.h>
#include <tb_system.h>

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO
uint32 dbg_bitmap_validations = 0;
#endif

GLuint g_current_texture = (GLuint) -1;
TBRendererBatcher::Batch *g_current_batch = 0;

void BindBitmap(TBBitmap *bitmap) {
	GLuint texture = bitmap ? static_cast<UIBitmapGL*>(bitmap)->m_texture : 0;
	if (texture != g_current_texture) {
		g_current_texture = texture;
		glBindTexture(GL_TEXTURE_2D, g_current_texture);
	}
}

UIBitmapGL::UIBitmapGL(UIRendererGL *renderer) :
		m_renderer(renderer), m_w(0), m_h(0), m_texture(0) {
}

UIBitmapGL::~UIBitmapGL() {
	m_renderer->FlushBitmap(this);
	if (m_texture == g_current_texture)
		BindBitmap(nullptr);

	glDeleteTextures(1, &m_texture);
}

bool UIBitmapGL::Init(int width, int height, uint32 *data) {
	assert(width == TBGetNearestPowerOfTwo(width));
	assert(height == TBGetNearestPowerOfTwo(height));

	m_w = width;
	m_h = height;

	glGenTextures(1, &m_texture);
	BindBitmap(this);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GL_checkError();

	SetData(data);

	return true;
}

void UIBitmapGL::SetData(uint32 *data) {
	m_renderer->FlushBitmap(this);
	BindBitmap(this);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	TB_IF_DEBUG_SETTING(RENDER_BATCHES, dbg_bitmap_validations++);
}

UIRendererGL::UIRendererGL() : _buffer(0) {
}

bool UIRendererGL::init() {
	if (!_shader.loadProgram("shaders/ui"))
		return false;
	glGenBuffers(1, &_buffer);
	GL_checkError();
	return true;
}

void UIRendererGL::BeginPaint(int render_target_w, int render_target_h) {
#ifdef TB_RUNTIME_DEBUG_INFO
	dbg_bitmap_validations = 0;
#endif

	TBRendererBatcher::BeginPaint(render_target_w, render_target_h);

	_shader.activate();

	const glm::mat4& ortho = glm::ortho(0.0f, (float) render_target_w, (float) render_target_h, 0.0f, -1.0f, 1.0f);
	_shader.setUniformMatrix("u_projection", ortho, false);

	g_current_texture = (GLuint) -1;
	g_current_batch = nullptr;

	glViewport(0, 0, render_target_w, render_target_h);
	glScissor(0, 0, render_target_w, render_target_h);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_checkError();
}

void UIRendererGL::EndPaint() {
	TBRendererBatcher::EndPaint();
	_shader.deactivate();
	glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES))
		TBDebugPrint("Frame caused %d bitmap validations.\n", dbg_bitmap_validations);
#endif
	GL_checkError();
}

TBBitmap *UIRendererGL::CreateBitmap(int width, int height, uint32 *data) {
	UIBitmapGL *bitmap = new UIBitmapGL(this);
	if (!bitmap || !bitmap->Init(width, height, data)) {
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

void UIRendererGL::RenderBatch(Batch *batch) {
	BindBitmap(batch->bitmap);
	if (g_current_batch != batch) {
		g_current_batch = batch;
	}
	glBindBuffer(GL_ARRAY_BUFFER, _buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * batch->vertex_count, batch->vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(_shader.enableVertexAttribute("a_color"), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Batch, vertex[0].col)));
	glVertexAttribPointer(_shader.enableVertexAttribute("a_texcoord"), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Batch, vertex[0].u)));
	glVertexAttribPointer(_shader.enableVertexAttribute("a_pos"), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Batch, vertex[0].x)));

	glDrawArrays(GL_TRIANGLES, 0, batch->vertex_count);
	GL_checkError();
}

void UIRendererGL::SetClipRect(const TBRect &rect) {
	glScissor(m_clip_rect.x, m_screen_rect.h - (m_clip_rect.y + m_clip_rect.h), m_clip_rect.w, m_clip_rect.h);
	GL_checkError();
}

}
