/**
 * @file
 */

#pragma once

#include "CanvasShader.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "image/FontAtlas.h"
#include "video/Buffer.h"
#include "video/Texture.h"
#include "math/Vec4f.h"
#include "math/Vec2f.h"

namespace render {

/**
 * Native 2D drawing facade. Logical pixels, top-left origin,
 * explicit clipping, no UI toolkit dependency.
 */
class CanvasRenderer {
private:
	struct Vertex {
		math::Vec2f pos, texcoord;
		math::Vec4f color, clip;
	};
	struct Batch {
		video::TexturePtr texture;
		core::Buffer<Vertex> vertices;
	};
	shader::CanvasShader _shader;
	video::Buffer _buffer;
	int _vertices = -1;
	image::FontAtlas _font;
	video::TexturePtr _fontTexture;
	math::Vec2f _solidUV{0};
	core::DynamicArray<Batch> _batches;
	math::Vec2f _size{1};
	math::Vec4f _clip{0};
	core::DynamicArray<math::Vec4f> _clips;
	void triangle(const math::Vec2f &a, const math::Vec2f &b, const math::Vec2f &c, const math::Vec4f &color);
	Batch &batch(const video::TexturePtr &texture);

public:
	bool init(const char *font = "font.ttf", uint32_t lastCodepoint = 255);
	void shutdown();
	void begin(const math::Vec2f &logicalSize);
	void clip(const math::Vec4f &rect);
	void resetClip();
	bool pushClip(const math::Vec4f &rect);
	bool popClip();
	const math::Vec4f &clipRect() const;
	void rect(const math::Vec4f &rect, const math::Vec4f &color, float radius = 0);
	void line(const math::Vec2f &a, const math::Vec2f &b, const math::Vec4f &color, float width = 1);
	void circle(const math::Vec2f &center, float radius, const math::Vec4f &color, float width = 1);
	void text(const math::Vec2f &position, const char *text, const math::Vec4f &color, float height = 18);
	float textWidth(const char *text, float height = 18) const;
	void image(const math::Vec4f &rect, const video::TexturePtr &texture, const math::Vec4f &uv = math::Vec4f(0, 0, 1, 1));

	/** Flush into the current target and viewport; normalized coordinates also support high-DPI targets. */
	void render();
	static bool contains(const math::Vec4f &rect, const math::Vec2f &point);
};

inline const math::Vec4f &CanvasRenderer::clipRect() const {
	return _clip;
}

inline float CanvasRenderer::textWidth(const char *text, float height) const {
	return _font.width(text, height);
}

} // namespace render
