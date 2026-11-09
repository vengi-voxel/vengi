/**
 * @file
 */

#include "FontAtlas.h"
#include "app/App.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/Unicode.h"
#include "image/Image.h"
#include "io/Filesystem.h"
#include <glm/common.hpp>
#define STBTT_assert core_assert
#define STBTT_malloc(x, u) ((void)(u), core_malloc(x))
#define STBTT_free(x, u) ((void)(u), core_free(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

namespace image {

FontAtlas::FontAtlas() = default;

FontAtlas::~FontAtlas() {
	shutdown();
}

bool FontAtlas::init(const char *filename, float height, uint32_t lastCodepoint) {
	shutdown();
	if (glm::isinf(height) || height < 4.0f || height > 64.0f || lastCodepoint < 127 || lastCodepoint > 4095) {
		return false;
	}
	const auto file = io::filesystem()->open(filename);
	if (!file->exists() || file->length() < 12) {
		return false;
	}
	file->read((void **)&_ttfBuffer);
	if (!_ttfBuffer) {
		return false;
	}
	const int offset = stbtt_GetFontOffsetForIndex(_ttfBuffer, 0);
	_font = new stbtt_fontinfo();
	if (!stbtt_InitFont(_font, _ttfBuffer, offset)) {
		shutdown();
		return false;
	}
	const int size = lastCodepoint > 1023 ? 2048 : 1024;
	uint8_t *pixels = (uint8_t *)core_malloc(size * size);
	core::DynamicArray<stbtt_bakedchar> metrics(lastCodepoint - 31);
	const int result =
		stbtt_BakeFontBitmap(_ttfBuffer, 0, height, pixels, size, size, 32, (int)metrics.size(), metrics.data());
	if (result <= 0) {
		core_free(pixels);
		shutdown();
		return false;
	}
	_height = height;
	_glyphs.resize(metrics.size());
	for (size_t i = 0; i < metrics.size(); ++i) {
		const auto &g = metrics[i];
		_glyphs[i] = {glm::vec4(g.x0, g.y0, g.x1, g.y1) / (float)size,
					  glm::vec4(g.xoff, g.yoff + height * 0.8f, g.x1 - g.x0, g.y1 - g.y0), g.xadvance};
	}
	_image = createEmptyImage("font atlas");
	_image->load(size, size, [pixels, size](int x, int y, color::RGBA &rgba) {
		rgba = color::RGBA(255, 255, 255, pixels[y * size + x]);
	});
	core_free(pixels);
	return true;
}

void FontAtlas::shutdown() {
	_image.release();
	_glyphs.clear();
	delete _font;
	_font = nullptr;
	delete[] _ttfBuffer;
	_ttfBuffer = nullptr;
}

const FontAtlas::Glyph &FontAtlas::glyph(uint32_t c) const {
	static const Glyph empty;
	if (_glyphs.empty()) {
		return empty;
	}
	return _glyphs[c >= 32 && c - 32 < _glyphs.size() ? c - 32 : '?' - 32];
}

float FontAtlas::width(const char *text, float height) const {
	float x = 0;
	float maximum = 0;
	for (int c = core::unicode::next(&text); c != -1; c = core::unicode::next(&text)) {
		if (c == '\n') {
			maximum = glm::max(maximum, x);
			x = 0;
		} else {
			x += glyph(c).advance * height / _height;
		}
	}
	return glm::max(maximum, x);
}

bool FontAtlas::rasterize(uint32_t codepoint, float size, uint8_t *&bitmap, int &w, int &h, int &x0, int &y0) const {
	bitmap = nullptr;
	w = 0;
	h = 0;
	x0 = 0;
	y0 = 0;
	if (_font == nullptr || glm::isinf(size) || size <= 0.0f) {
		return false;
	}
	const float scale = stbtt_ScaleForPixelHeight(_font, size);
	bitmap = stbtt_GetCodepointBitmap(_font, scale, scale, (int)codepoint, &w, &h, &x0, &y0);
	return bitmap != nullptr;
}

void FontAtlas::freeBitmap(uint8_t *bitmap) const {
	stbtt_FreeBitmap(bitmap, nullptr);
}

void FontAtlas::measure(const char *string, float size, int &w, int &h) const {
	w = 0;
	h = 0;
	if (_font == nullptr || string == nullptr || glm::isinf(size) || size <= 0.0f) {
		return;
	}
	const float scale = stbtt_ScaleForPixelHeight(_font, size);
	for (int c = core::unicode::next(&string); c != -1; c = core::unicode::next(&string)) {
		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(_font, c, scale, scale, &ix0, &iy0, &ix1, &iy1);
		w += (ix1 - ix0);
		h = core_max(h, iy1 - iy0);
	}
}

} // namespace image
