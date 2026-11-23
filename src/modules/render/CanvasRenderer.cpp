/**
 * @file
 */

#include "CanvasRenderer.h"
#include "core/Unicode.h"
#include "image/FontAtlas.h"
#include "image/Image.h"
#include "video/Buffer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedState.h"
#include "video/Texture.h"
#include "math/Conversions.h"
#include "math/Vec4f.h"
#include "math/Vec2f.h"

namespace render {

bool CanvasRenderer::init(const char *font, uint32_t lastCodepoint) {
	if (!_shader.setup() || !_font.init(font, 32, lastCodepoint)) {
		return false;
	}
	_font.image()->setColor(color::RGBA(255, 255, 255, 255), 0, 0);
	_solidUV = math::Vec2f(0.5f / _font.image()->width(), 0.5f / _font.image()->height());
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

void CanvasRenderer::begin(const math::Vec2f &size) {
	_size = math::max(size, math::Vec2f(1));
	_batches.clear();
	resetClip();
}

void CanvasRenderer::clip(const math::Vec4f &r) {
	const math::Vec2f low = math::clamp(math::Vec2f(r), math::Vec2f(0), _size);
	const math::Vec2f high = math::clamp(math::Vec2f(r) + math::max(math::Vec2f(r.z, r.w), math::Vec2f(0)), low, _size);
	_clip = math::Vec4f(low, high - low);
}

void CanvasRenderer::resetClip() {
	_clips.clear();
	_clip = math::Vec4f(0, 0, _size);
}

bool CanvasRenderer::pushClip(const math::Vec4f &rect) {
	if (_clips.size() >= 32) {
		return false;
	}
	_clips.push_back(_clip);
	const math::Vec2f low = math::max(math::Vec2f(_clip), math::Vec2f(rect));
	const math::Vec2f high = math::min(math::Vec2f(_clip) + math::Vec2f(_clip.z, _clip.w),
									math::Vec2f(rect) + math::max(math::Vec2f(rect.z, rect.w), math::Vec2f(0)));
	_clip = math::Vec4f(low, math::max(high - low, math::Vec2f(0)));
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

bool CanvasRenderer::contains(const math::Vec4f &r, const math::Vec2f &p) {
	return p.x >= r.x && p.y >= r.y && p.x < r.x + r.z && p.y < r.y + r.w;
}

CanvasRenderer::Batch &CanvasRenderer::batch(const video::TexturePtr &texture) {
	if (_batches.empty() || _batches.back().texture != texture) {
		_batches.push_back(Batch{texture, {}});
	}
	return _batches.back();
}

void CanvasRenderer::triangle(const math::Vec2f &a, const math::Vec2f &b, const math::Vec2f &c, const math::Vec4f &color) {
	core::Buffer<Vertex> &vertices = batch(_fontTexture).vertices;
	for (const math::Vec2f &p : {a, b, c}) {
		math::Vec2f position(p.x * 2 / _size.x - 1, 1 - p.y * 2 / _size.y);
		math::Vec4f clip(_clip.x / _size.x, _clip.y / _size.y, (_clip.x + _clip.z) / _size.x,
					   (_clip.y + _clip.w) / _size.y);
		Vertex vertex{position, _solidUV, color, clip};
		vertices.emplace_back(vertex);
	}
}

void CanvasRenderer::rect(const math::Vec4f &r, const math::Vec4f &color, float radius) {
	if (r.z <= 0 || r.w <= 0) {
		return;
	}
	radius = math::clamp(radius, 0.0f, math::min(r.z, r.w) * 0.5f);
	const math::Vec2f center(r.x + r.z * 0.5f, r.y + r.w * 0.5f);
	math::Vec2f previous(r.x + r.z - radius, r.y);
	for (int corner = 0; corner < 4; ++corner) {
		const math::Vec2f origin(r.x + (corner < 2 ? r.z - radius : radius),
							   r.y + (corner == 0 || corner == 3 ? radius : r.w - radius));
		for (int step = 0; step <= 8; ++step) {
			const float angle = (-1.570796327f + corner * 1.570796327f) + step * 1.570796327f / 8;
			const math::Vec2f p = origin + math::Vec2f(math::cos(angle), math::sin(angle)) * radius;
			triangle(center, previous, p, color);
			previous = p;
		}
	}
	triangle(center, previous, math::Vec2f(r.x + r.z - radius, r.y), color);
}

void CanvasRenderer::line(const math::Vec2f &a, const math::Vec2f &b, const math::Vec4f &color, float width) {
	const math::Vec2f delta = b - a;
	const float length = math::length(delta);
	if (length < 0.001f || width <= 0) {
		return;
	}
	const math::Vec2f side(-delta.y / length * width * 0.5f, delta.x / length * width * 0.5f);
	triangle(a - side, a + side, b + side, color);
	triangle(a - side, b + side, b - side, color);
}

void CanvasRenderer::circle(const math::Vec2f &center, float radius, const math::Vec4f &color, float width) {
	math::Vec2f previous = center + math::Vec2f(radius, 0);
	const int steps = 64;
	for (int i = 1; i <= steps; ++i) {
		const float angle = i * 6.283185307f / (float)steps;
		const math::Vec2f p = center + radius * math::Vec2f(math::cos(angle), math::sin(angle));
		line(previous, p, color, width);
		previous = p;
	}
}

void CanvasRenderer::image(const math::Vec4f &r, const video::TexturePtr &texture, const math::Vec4f &uv) {
	core::Buffer<Vertex> &vertices = batch(texture).vertices;
	const int corners[] = {0, 1, 2, 2, 3, 0};
	const math::Vec2f pos[] = {{r.x, r.y}, {r.x + r.z, r.y}, {r.x + r.z, r.y + r.w}, {r.x, r.y + r.w}};
	const math::Vec2f tex[] = {{uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w}, {uv.x, uv.w}};
	for (int i : corners) {
		math::Vec2f position(pos[i].x * 2 / _size.x - 1, 1 - pos[i].y * 2 / _size.y);
		math::Vec4f clip(_clip.x / _size.x, _clip.y / _size.y, (_clip.x + _clip.z) / _size.x,
					   (_clip.y + _clip.w) / _size.y);
		Vertex vertex{position, tex[i], math::Vec4f(1.0f), clip};
		vertices.emplace_back(vertex);
	}
}

void CanvasRenderer::text(const math::Vec2f &position, const char *text, const math::Vec4f &color, float height) {
	const float scale = height / _font.height();
	math::Vec2f pen = position;
	for (int c = core::unicode::next(&text); c != -1; c = core::unicode::next(&text)) {
		if (c == '\n') {
			pen.x = position.x;
			pen.y += height * 1.25f;
			continue;
		}
		const image::FontAtlas::Glyph &g = _font.glyph(c);
		image(math::Vec4f(pen + math::Vec2f(g.bounds) * scale, g.bounds.z * scale, g.bounds.w * scale), _fontTexture, g.uv);
		core::Buffer<Vertex> &vertices = _batches.back().vertices;
		for (size_t i = vertices.size() - 6; i < vertices.size(); ++i) {
			vertices[i].color = color;
		}
		pen.x += g.advance * scale;
	}
}

void CanvasRenderer::render() {
	const video::ScopedState depth(video::State::DepthTest, false);
	const video::ScopedState mask(video::State::DepthMask, false);
	const video::ScopedState cull(video::State::CullFace, false);
	const video::ScopedState scissor(video::State::Scissor, false);
	const video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
									   video::BlendEquation::Add);
	video::ScopedShader shader(_shader);
	for (const Batch &batch : _batches) {
		video::ScopedTexture texture(batch.texture, video::TextureUnit::Zero);
		_buffer.update(_vertices, batch.vertices.data(), batch.vertices.size() * sizeof(Vertex), true);
		video::ScopedBuffer buffer(_buffer);
		video::drawArrays(video::Primitive::Triangles, batch.vertices.size());
	}
}

} // namespace render
