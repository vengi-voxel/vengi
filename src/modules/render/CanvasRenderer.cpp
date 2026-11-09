/**
 * @file
 */
#include "CanvasRenderer.h"
#include "core/Unicode.h"
#include "image/Image.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedState.h"
#include <glm/geometric.hpp>

namespace render {

bool CanvasRenderer::init(const char *font, uint32_t lastCodepoint) {
	if (!_shader.setup() || !_font.init(font, 32, lastCodepoint))
		return false;
	_font.image()->setColor(color::RGBA(255, 255, 255, 255), 0, 0);
	_solidUV = glm::vec2(0.5f / _font.image()->width(), 0.5f / _font.image()->height());
	_fontTexture = video::createTextureFromImage(_font.image());
	_vertices = _buffer.create();
	_buffer.setMode(_vertices, video::BufferMode::Stream);
	return _vertices >= 0 && _buffer.addAttribute(_shader.getPosAttribute(_vertices, &Vertex::pos)) &&
		   _buffer.addAttribute(_shader.getTexcoordAttribute(_vertices, &Vertex::texcoord)) &&
		   _buffer.addAttribute(_shader.getColorAttribute(_vertices, &Vertex::color)) &&
		   _buffer.addAttribute(_shader.getClipAttribute(_vertices, &Vertex::clip));
}

void CanvasRenderer::shutdown() {
	_batches.clear();
	_fontTexture.release();
	_font.shutdown();
	_buffer.shutdown();
	_shader.shutdown();
	_vertices = -1;
}

void CanvasRenderer::begin(const glm::vec2 &size) {
	_size = glm::max(size, glm::vec2(1));
	_batches.clear();
	resetClip();
}

void CanvasRenderer::clip(const glm::vec4 &r) {
	const glm::vec2 low = glm::clamp(glm::vec2(r), glm::vec2(0), _size);
	const glm::vec2 high = glm::clamp(glm::vec2(r) + glm::max(glm::vec2(r.z, r.w), glm::vec2(0)), low, _size);
	_clip = glm::vec4(low, high - low);
}

void CanvasRenderer::resetClip() {
	_clips.clear();
	_clip = glm::vec4(0, 0, _size);
}

bool CanvasRenderer::pushClip(const glm::vec4 &rect) {
	if (_clips.size() >= 32) {
		return false;
	}
	_clips.push_back(_clip);
	const glm::vec2 low = glm::max(glm::vec2(_clip), glm::vec2(rect));
	const glm::vec2 high = glm::min(glm::vec2(_clip) + glm::vec2(_clip.z, _clip.w),
									glm::vec2(rect) + glm::max(glm::vec2(rect.z, rect.w), glm::vec2(0)));
	_clip = glm::vec4(low, glm::max(high - low, glm::vec2(0)));
	return true;
}

bool CanvasRenderer::popClip() {
	if (_clips.empty()) {
		return false;
	}
	_clip = _clips.back();
	_clips.pop();
	return true;
}

bool CanvasRenderer::contains(const glm::vec4 &r, const glm::vec2 &p) {
	return p.x >= r.x && p.y >= r.y && p.x < r.x + r.z && p.y < r.y + r.w;
}

CanvasRenderer::Batch &CanvasRenderer::batch(const video::TexturePtr &texture) {
	if (_batches.empty() || _batches.back().texture != texture) {
		_batches.push_back(Batch{texture, {}});
	}
	return _batches.back();
}

void CanvasRenderer::triangle(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c, const glm::vec4 &color) {
	core::Buffer<Vertex> &vertices = batch(_fontTexture).vertices;
	for (const glm::vec<2, float> &p : {a, b, c}) {
		Vertex vertex{glm::vec2(p.x * 2 / _size.x - 1, 1 - p.y * 2 / _size.y), _solidUV, color,
					  glm::vec4(_clip.x / _size.x, _clip.y / _size.y, (_clip.x + _clip.z) / _size.x,
								(_clip.y + _clip.w) / _size.y)};
		vertices.emplace_back(vertex);
	}
}

void CanvasRenderer::rect(const glm::vec4 &r, const glm::vec4 &color, float radius) {
	if (r.z <= 0 || r.w <= 0) {
		return;
	}
	radius = glm::clamp(radius, 0.0f, glm::min(r.z, r.w) * 0.5f);
	const glm::vec2 center(r.x + r.z * 0.5f, r.y + r.w * 0.5f);
	glm::vec2 previous(r.x + r.z - radius, r.y);
	for (int corner = 0; corner < 4; ++corner) {
		const glm::vec2 origin(r.x + (corner < 2 ? r.z - radius : radius),
							   r.y + (corner == 0 || corner == 3 ? radius : r.w - radius));
		for (int step = 0; step <= 8; ++step) {
			const float angle = (-1.570796327f + corner * 1.570796327f) + step * 1.570796327f / 8;
			const glm::vec2 p = origin + glm::vec2(cosf(angle), sinf(angle)) * radius;
			triangle(center, previous, p, color);
			previous = p;
		}
	}
	triangle(center, previous, glm::vec2(r.x + r.z - radius, r.y), color);
}

void CanvasRenderer::line(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &color, float width) {
	const glm::vec2 delta = b - a;
	const float length = glm::length(delta);
	if (length < 0.001f || width <= 0) {
		return;
	}
	const glm::vec2 side(-delta.y / length * width * 0.5f, delta.x / length * width * 0.5f);
	triangle(a - side, a + side, b + side, color);
	triangle(a - side, b + side, b - side, color);
}

void CanvasRenderer::circle(const glm::vec2 &center, float radius, const glm::vec4 &color, float width) {
	glm::vec2 previous = center + glm::vec2(radius, 0);
	const int steps = 64;
	for (int i = 1; i <= steps; ++i) {
		const float angle = i * 6.283185307f / (float)steps;
		const glm::vec2 p = center + radius * glm::vec2(cosf(angle), sinf(angle));
		line(previous, p, color, width);
		previous = p;
	}
}

void CanvasRenderer::image(const glm::vec4 &r, const video::TexturePtr &texture, const glm::vec4 &uv) {
	core::Buffer<Vertex> &vertices = batch(texture).vertices;
	const int corners[] = {0, 1, 2, 2, 3, 0};
	const glm::vec2 pos[] = {{r.x, r.y}, {r.x + r.z, r.y}, {r.x + r.z, r.y + r.w}, {r.x, r.y + r.w}};
	const glm::vec2 tex[] = {{uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w}, {uv.x, uv.w}};
	for (int i : corners) {
		glm::vec2 position(pos[i].x * 2 / _size.x - 1, 1 - pos[i].y * 2 / _size.y);
		glm::vec4 clip(_clip.x / _size.x, _clip.y / _size.y, (_clip.x + _clip.z) / _size.x,
					   (_clip.y + _clip.w) / _size.y);
		Vertex vertex{position, tex[i], glm::vec4(1.0f), clip};
		vertices.emplace_back(vertex);
	}
}

void CanvasRenderer::text(const glm::vec2 &position, const char *text, const glm::vec4 &color, float height) {
	const float scale = height / _font.height();
	glm::vec2 pen = position;
	for (int c = core::unicode::next(&text); c != -1; c = core::unicode::next(&text)) {
		if (c == '\n') {
			pen.x = position.x;
			pen.y += height * 1.25f;
			continue;
		}
		const auto &g = _font.glyph(c);
		image(glm::vec4(pen + glm::vec2(g.bounds) * scale, g.bounds.z * scale, g.bounds.w * scale), _fontTexture, g.uv);
		auto &vertices = _batches.back().vertices;
		for (size_t i = vertices.size() - 6; i < vertices.size(); ++i)
			vertices[i].color = color;
		pen.x += g.advance * scale;
	}
}

void CanvasRenderer::render() {
	const video::ScopedState depth(video::State::DepthTest, false), mask(video::State::DepthMask, false),
		cull(video::State::CullFace, false), scissor(video::State::Scissor, false);
	const video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
									   video::BlendEquation::Add);
	_shader.activate();
	for (const Batch &batch : _batches) {
		video::bindTexture(video::TextureUnit::Zero, batch.texture);
		_buffer.update(_vertices, batch.vertices.data(), batch.vertices.size() * sizeof(Vertex), true);
		_buffer.bind();
		video::drawArrays(video::Primitive::Triangles, batch.vertices.size());
		_buffer.unbind();
	}
	_shader.deactivate();
}

} // namespace render
