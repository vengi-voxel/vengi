/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "image/ImageFwd.h"
#include <glm/vec4.hpp>
#include <stdint.h>

struct stbtt_fontinfo;

namespace image {

/** Rasterized font metrics are independent of a graphics context. Coordinates use a top-left origin. */
class FontAtlas {
public:
	struct Glyph {
		glm::vec4 uv{0};
		glm::vec4 bounds{0};
		float advance = 0;
	};

private:
	core::DynamicArray<Glyph> _glyphs;
	ImagePtr _image;
	stbtt_fontinfo *_font = nullptr;
	uint8_t *_ttfBuffer = nullptr;
	float _height = 32;

public:
	FontAtlas();
	~FontAtlas();

	bool init(const char *filename, float height = 32, uint32_t lastCodepoint = 255);
	void shutdown();

	const Glyph &glyph(uint32_t c) const;

	const ImagePtr &image() const {
		return _image;
	}
	float height() const {
		return _height;
	}
	float width(const char *text, float height) const;

	bool rasterize(uint32_t codepoint, float size, uint8_t *&bitmap, int &w, int &h, int &x0, int &y0) const;
	void freeBitmap(uint8_t *bitmap) const;
	void measure(const char *string, float size, int &w, int &h) const;
};

} // namespace image
