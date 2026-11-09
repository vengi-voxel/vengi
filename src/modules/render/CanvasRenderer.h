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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace render {

/** Native 2D drawing facade. Logical pixels, top-left origin, explicit clipping, no UI toolkit dependency. */
class CanvasRenderer {
private:
	struct Vertex {
		glm::vec2 pos, texcoord;
		glm::vec4 color, clip;
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
	glm::vec2 _solidUV{0};
	core::DynamicArray<Batch> _batches;
	glm::vec2 _size{1};
	glm::vec4 _clip{0};
	core::DynamicArray<glm::vec4> _clips;
	void triangle(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c, const glm::vec4 &color);
	Batch &batch(const video::TexturePtr &texture);

public:
	bool init(const char *font = "font.ttf", uint32_t lastCodepoint = 255);
	void shutdown();
	void begin(const glm::vec2 &logicalSize);
	void clip(const glm::vec4 &rect);
	void resetClip();
	bool pushClip(const glm::vec4 &rect);
	bool popClip();
	const glm::vec4 &clipRect() const {
		return _clip;
	}
	void rect(const glm::vec4 &rect, const glm::vec4 &color, float radius = 0);
	void line(const glm::vec2 &a, const glm::vec2 &b, const glm::vec4 &color, float width = 1);
	void circle(const glm::vec2 &center, float radius, const glm::vec4 &color, float width = 1);
	void text(const glm::vec2 &position, const char *text, const glm::vec4 &color, float height = 18);
	float textWidth(const char *text, float height = 18) const {
		return _font.width(text, height);
	}
	void image(const glm::vec4 &rect, const video::TexturePtr &texture, const glm::vec4 &uv = glm::vec4(0, 0, 1, 1));
	/** Flush into the current target and viewport; normalized coordinates also support high-DPI targets. */
	void render();
	static bool contains(const glm::vec4 &rect, const glm::vec2 &point);
};

} // namespace render
