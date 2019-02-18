/**
 * @file
 */

#include "tb_font_renderer.h"
#include "tb_renderer.h"
#include "tb_skin.h"
#include "tb_system.h"
#include <math.h>

namespace tb {

static void blurGlyph(const unsigned char *src, int srcw, int srch, int srcStride, unsigned char *dst, int dstw,
					  int dsth, int dstStride, float *temp, const float *kernel, int kernelRadius) {
	for (int y = 0; y < srch; y++) {
		for (int x = 0; x < dstw; x++) {
			float val = 0;
			for (int k_ofs = -kernelRadius; k_ofs <= kernelRadius; k_ofs++) {
				if (x - kernelRadius + k_ofs >= 0 && x - kernelRadius + k_ofs < srcw) {
					val += src[y * srcStride + x - kernelRadius + k_ofs] * kernel[k_ofs + kernelRadius];
				}
			}
			temp[y * dstw + x] = val;
		}
	}
	for (int y = 0; y < dsth; y++) {
		for (int x = 0; x < dstw; x++) {
			float val = 0;
			for (int k_ofs = -kernelRadius; k_ofs <= kernelRadius; k_ofs++) {
				if (y - kernelRadius + k_ofs >= 0 && y - kernelRadius + k_ofs < srch) {
					val += temp[(y - kernelRadius + k_ofs) * dstw + x] * kernel[k_ofs + kernelRadius];
				}
			}
			dst[y * dstStride + x] = (unsigned char)(val + 0.5F);
		}
	}
}

// ================================================================================================

void TBFontEffect::setBlurRadius(int blurRadius) {
	core_assert(blurRadius >= 0);
	if (m_blur_radius == blurRadius) {
		return;
	}
	m_blur_radius = blurRadius;
	if (m_blur_radius > 0) {
		if (!m_kernel.reserve(sizeof(float) * (m_blur_radius * 2 + 1))) {
			m_blur_radius = 0;
			return;
		}
		float *kernel = (float *)m_kernel.getData();
		float stdDevSq2 = (float)m_blur_radius / 2.F;
		stdDevSq2 = 2.F * stdDevSq2 * stdDevSq2;
		float scale = 1.F / sqrtf(3.1415F * stdDevSq2);
		float sum = 0;
		for (int k = 0; k < 2 * m_blur_radius + 1; k++) {
			float x = (float)(k - m_blur_radius);
			float kval = scale * expf(-(x * x / stdDevSq2));
			kernel[k] = kval;
			sum += kval;
		}
		for (int k = 0; k < 2 * m_blur_radius + 1; k++) {
			kernel[k] /= sum;
		}
	}
}

TBFontGlyphData *TBFontEffect::render(TBGlyphMetrics *metrics, const TBFontGlyphData *src) {
	TBFontGlyphData *effect_glyph_data = nullptr;
	if (m_blur_radius > 0 && (src->data8 != nullptr)) {
		// Create a new TBFontGlyphData for the blurred glyph
		effect_glyph_data = new TBFontGlyphData;
		if (effect_glyph_data == nullptr) {
			return nullptr;
		}
		effect_glyph_data->w = src->w + m_blur_radius * 2;
		effect_glyph_data->h = src->h + m_blur_radius * 2;
		effect_glyph_data->stride = effect_glyph_data->w;

		// Reserve memory needed for blurring.
		if (!m_data_dst.reserve(effect_glyph_data->w * effect_glyph_data->h) ||
			!m_blur_temp.reserve(effect_glyph_data->w * effect_glyph_data->h * sizeof(float))) {
			delete effect_glyph_data;
			return nullptr;
		}
		effect_glyph_data->data8 = (uint8_t *)m_data_dst.getData();

		// Blur!
		blurGlyph(src->data8, src->w, src->h, src->stride, effect_glyph_data->data8, effect_glyph_data->w,
				  effect_glyph_data->h, effect_glyph_data->w, (float *)m_blur_temp.getData(),
				  (float *)m_kernel.getData(), m_blur_radius);

		// Adjust glyph position to compensate for larger size.
		metrics->x -= m_blur_radius;
		metrics->y -= m_blur_radius;
	}
	return effect_glyph_data;
}

// == TBFontGlyph =================================================================================

TBFontGlyph::TBFontGlyph(const TBID &hashId, UCS4 cp) : hash_id(hashId), cp(cp), frag(nullptr), has_rgb(false) {
}

// == TBFontGlyphCache ============================================================================

TBFontGlyphCache::TBFontGlyphCache() {
	// Only use one map for the font face. The glyph cache will start forgetting
	// glyphs that haven't been used for a while if the map gets full.
	m_frag_manager.setNumMapsLimit(1);
	m_frag_manager.setDefaultMapSize(TB_GLYPH_CACHE_WIDTH, TB_GLYPH_CACHE_HEIGHT);

	g_renderer->addListener(this);
}

TBFontGlyphCache::~TBFontGlyphCache() {
	g_renderer->removeListener(this);
}

TBFontGlyph *TBFontGlyphCache::getGlyph(const TBID &hashId, UCS4 cp) {
	if (TBFontGlyph *glyph = m_glyphs.get(hashId)) {
		// Move the glyph to the end of m_all_rendered_glyphs so we maintain LRU (oldest first)
		if (m_all_rendered_glyphs.containsLink(glyph)) {
			m_all_rendered_glyphs.remove(glyph);
			m_all_rendered_glyphs.addLast(glyph);
		}
		return glyph;
	}
	return nullptr;
}

TBFontGlyph *TBFontGlyphCache::createAndCacheGlyph(const TBID &hashId, UCS4 cp) {
	core_assert(!getGlyph(hashId, cp));
	TBFontGlyph *glyph = new TBFontGlyph(hashId, cp);
	if ((glyph != nullptr) && m_glyphs.add(glyph->hash_id, glyph)) {
		return glyph;
	}
	delete glyph;
	return nullptr;
}

TBBitmapFragment *TBFontGlyphCache::createFragment(TBFontGlyph *glyph, int w, int h, int stride, uint32_t *data) {
	core_assert(getGlyph(glyph->hash_id, glyph->cp));
	// Don't bother if the requested glyph is too large.
	if (w > TB_GLYPH_CACHE_WIDTH || h > TB_GLYPH_CACHE_HEIGHT) {
		return nullptr;
	}

	bool try_drop_largest = true;
	bool dropped_large_enough_glyph = false;
	do {
		// Attempt creating a fragment for the rendered glyph data
		if (TBBitmapFragment *frag = m_frag_manager.createNewFragment(glyph->hash_id, false, w, h, stride, data)) {
			glyph->frag = frag;
			m_all_rendered_glyphs.addLast(glyph);
			return frag;
		}
		// Drop the oldest glyph that's large enough to free up the space we need.
		if (try_drop_largest) {
			const int check_limit = 20;
			int check_count = 0;
			for (TBFontGlyph *oldest = m_all_rendered_glyphs.getFirst();
				 (oldest != nullptr) && check_count < check_limit; oldest = oldest->getNext()) {
				if (oldest->frag->width() >= w && oldest->frag->getAllocatedHeight() >= h) {
					dropGlyphFragment(oldest);
					dropped_large_enough_glyph = true;
					break;
				}
				check_count++;
			}
			try_drop_largest = false;
		}
		// We had no large enough glyph so just drop the oldest one. We will likely
		// spin around the loop, fail and drop again a few times before we succeed.
		if (!dropped_large_enough_glyph) {
			if (TBFontGlyph *oldest = m_all_rendered_glyphs.getFirst()) {
				dropGlyphFragment(oldest);
			} else {
				break;
			}
		}
	} while (true);
	return nullptr;
}

void TBFontGlyphCache::dropGlyphFragment(TBFontGlyph *glyph) {
	core_assert(glyph->frag);
	m_frag_manager.freeFragment(glyph->frag);
	glyph->frag = nullptr;
	m_all_rendered_glyphs.remove(glyph);
}

#ifdef TB_RUNTIME_DEBUG_INFO
void TBFontGlyphCache::debug() {
	m_frag_manager.debug();
}
#endif // TB_RUNTIME_DEBUG_INFO

void TBFontGlyphCache::onContextLost() {
	m_frag_manager.deleteBitmaps();
}

void TBFontGlyphCache::onContextRestored() {
	// No need to do anything. The bitmaps will be created when drawing.
}

// ================================================================================================

TBFontFace::TBFontFace(TBFontGlyphCache *glyphCache, TBFontRenderer *renderer, const TBFontDescription &fontDesc)
	: m_glyph_cache(glyphCache), m_font_renderer(renderer), m_font_desc(fontDesc), m_bgFont(nullptr), m_bgX(0),
	  m_bgY(0) {
	if (m_font_renderer != nullptr) {
		m_metrics = m_font_renderer->getMetrics();
	} else {
		// Invent some metrics for the test font
		int size = m_font_desc.getSize();
		m_metrics.ascent = size - size / 4;
		m_metrics.descent = size / 4;
		m_metrics.height = size;
	}
}

TBFontFace::~TBFontFace() {
	// It would be nice to drop all glyphs we have live for this font face.
	// Now they only die when they get old and kicked out of the cache.
	// We currently don't drop any font faces either though (except on shutdown)
	delete m_font_renderer;
}

void TBFontFace::setBackgroundFont(TBFontFace *font, const TBColor &col, int xofs, int yofs) {
	m_bgFont = font;
	m_bgX = xofs;
	m_bgY = yofs;
	m_bgColor = col;
}

bool TBFontFace::renderGlyphs(const char *glyphStr, int glyphStrLen) {
	if (m_font_renderer == nullptr) {
		return true; // This is the test font
	}

	if (glyphStrLen == TB_ALL_TO_TERMINATION) {
		glyphStrLen = strlen(glyphStr);
	}

	bool has_all_glyphs = true;
	int i = 0;
	while ((glyphStr[i] != 0) && i < glyphStrLen) {
		UCS4 cp = utf8::decode_next(glyphStr, &i, glyphStrLen);
		if (getGlyph(cp, true) == nullptr) {
			has_all_glyphs = false;
		}
	}
	return has_all_glyphs;
}

TBFontGlyph *TBFontFace::createAndCacheGlyph(const TBID &hashId, UCS4 cp) {
	if (m_font_renderer == nullptr) {
		return nullptr; // This is the test font
	}

	// Create the new glyph
	TBFontGlyph *glyph = m_glyph_cache->createAndCacheGlyph(hashId, cp);
	if (glyph != nullptr) {
		m_font_renderer->getGlyphMetrics(&glyph->metrics, cp);
	}
	return glyph;
}

void TBFontFace::renderGlyph(TBFontGlyph *glyph) {
	core_assert(!glyph->frag);
	TBFontGlyphData glyph_data;
	if (m_font_renderer->renderGlyph(&glyph_data, glyph->cp)) {
		TBFontGlyphData *effect_glyph_data = m_effect.render(&glyph->metrics, &glyph_data);
		TBFontGlyphData *result_glyph_data = effect_glyph_data != nullptr ? effect_glyph_data : &glyph_data;

		// The glyph data may be in uint8 format, which we have to convert since we always
		// create fragments (and TBBitmap) in 32bit format.
		uint32_t *glyph_dsta_src = result_glyph_data->data32;
		if ((glyph_dsta_src == nullptr) && (result_glyph_data->data8 != nullptr)) {
			if (m_temp_buffer.reserve(result_glyph_data->w * result_glyph_data->h * sizeof(uint32_t))) {
				glyph_dsta_src = (uint32_t *)m_temp_buffer.getData();
				for (int y = 0; y < result_glyph_data->h; y++) {
					for (int x = 0; x < result_glyph_data->w; x++) {
#ifdef TB_PREMULTIPLIED_ALPHA
						uint8_t opacity = result_glyph_data->data8[x + y * result_glyph_data->stride];
						glyph_dsta_src[x + y * result_glyph_data->w] = TBColor(opacity, opacity, opacity, opacity);
#else
						glyph_dsta_src[x + y * result_glyph_data->w] =
							TBColor(255, 255, 255, result_glyph_data->data8[x + y * result_glyph_data->stride]);
#endif
					}
				}
			}
		}

		// Finally, the glyph data is ready and we can create a bitmap fragment.
		if (glyph_dsta_src != nullptr) {
			glyph->has_rgb = result_glyph_data->rgb;
			m_glyph_cache->createFragment(glyph, result_glyph_data->w, result_glyph_data->h, result_glyph_data->stride,
										  glyph_dsta_src);
		}

		delete effect_glyph_data;
	}
#ifdef TB_RUNTIME_DEBUG_INFO
	// char glyph_str[9];
	// int len = utf8::encode(cp, glyph_str);
	// glyph_str[len] = 0;
	// Log::debug("Created glyph %d (\"%s\"). Cache contains %d glyphs (%d%% full) using %d bitmaps.",
	// cp, glyph_str, m_all_glyphs.CountLinks(), m_frag_manager.getUseRatio(), m_frag_manager.getNumMaps());
#endif
}

TBID TBFontFace::getHashId(UCS4 cp) const {
	return cp * 3111 + m_font_desc.getFontFaceID();
}

TBFontGlyph *TBFontFace::getGlyph(UCS4 cp, bool renderIfNeeded) {
	const TBID &hash_id = getHashId(cp);
	TBFontGlyph *glyph = m_glyph_cache->getGlyph(hash_id, cp);
	if (glyph == nullptr) {
		glyph = createAndCacheGlyph(hash_id, cp);
	}
	if ((glyph != nullptr) && (glyph->frag == nullptr) && renderIfNeeded) {
		renderGlyph(glyph);
	}
	return glyph;
}

void TBFontFace::drawString(int x, int y, const TBColor &color, const char *str, int len) {
	if (m_bgFont != nullptr) {
		m_bgFont->drawString(x + m_bgX, y + m_bgY, m_bgColor, str, len);
	}

	if (m_font_renderer != nullptr) {
		g_renderer->beginBatchHint(TBRenderer::BATCH_HINT_DRAW_BITMAP_FRAGMENT);
	}

	int i = 0;
	while ((str[i] != 0) && i < len) {
		UCS4 cp = utf8::decode_next(str, &i, len);
		if (cp == 0xFFFF) {
			continue;
		}
		if (TBFontGlyph *glyph = getGlyph(cp, true)) {
			if (glyph->frag != nullptr) {
				TBRect dst_rect(x + glyph->metrics.x, y + glyph->metrics.y + getAscent(), glyph->frag->width(),
								glyph->frag->height());
				TBRect src_rect(0, 0, glyph->frag->width(), glyph->frag->height());
				if (glyph->has_rgb) {
					g_renderer->drawBitmap(dst_rect, src_rect, glyph->frag);
				} else {
					g_renderer->drawBitmapColored(dst_rect, src_rect, color, glyph->frag);
				}
			}
			x += glyph->metrics.advance;
		} else if (m_font_renderer == nullptr) // This is the test font. Use same glyph width as height and draw square.
		{
			g_tb_skin->paintRect(TBRect(x, y, m_metrics.height / 3, m_metrics.height), color, 1);
			x += m_metrics.height / 3 + 1;
		}
	}

	if (m_font_renderer != nullptr) {
		g_renderer->endBatchHint();
	}
}

int TBFontFace::getStringWidth(const char *str, int len) {
	int width = 0;
	int i = 0;
	while ((str[i] != 0) && i < len) {
		UCS4 cp = utf8::decode_next(str, &i, len);
		if (cp == 0xFFFF) {
			continue;
		}
		if (m_font_renderer == nullptr) { // This is the test font. Use same glyph width as height.
			width += m_metrics.height / 3 + 1;
		} else if (TBFontGlyph *glyph = getGlyph(cp, false)) {
			width += glyph->metrics.advance;
		}
	}
	return width;
}

#ifdef TB_RUNTIME_DEBUG_INFO
void TBFontFace::debug() {
	m_glyph_cache->debug();
}
#endif // TB_RUNTIME_DEBUG_INFO

// == TBFontManager ===============================================================================

TBFontManager::TBFontManager() {
	// Add the test dummy font with empty name (Equals to ID 0)
	addFontInfo("-test-font-dummy-", "");
	m_test_font_desc.setSize(16);
	createFontFace(m_test_font_desc);

	// Use the test dummy font as default by default
	m_default_font_desc = m_test_font_desc;
}

TBFontManager::~TBFontManager() {
}

TBFontInfo *TBFontManager::addFontInfo(const char *filename, const char *name) {
	if (TBFontInfo *fi = new TBFontInfo(filename, name)) {
		if (m_font_info.add(fi->getID(), fi)) {
			return fi;
		}
		delete fi;
	}
	return nullptr;
}

TBFontInfo *TBFontManager::getFontInfo(const TBID &id) const {
	return m_font_info.get(id);
}

bool TBFontManager::hasFontFace(const TBFontDescription &fontDesc) const {
	return m_fonts.get(fontDesc.getFontFaceID()) != nullptr;
}

TBFontFace *TBFontManager::getFontFace(const TBFontDescription &fontDesc) {
	if (TBFontFace *font = m_fonts.get(fontDesc.getFontFaceID())) {
		return font;
	}
	if (TBFontFace *font = m_fonts.get(getDefaultFontDescription().getFontFaceID())) {
		return font;
	}
	return m_fonts.get(m_test_font_desc.getFontFaceID());
}

TBFontFace *TBFontManager::createFontFace(const TBFontDescription &fontDesc) {
	core_assert(!hasFontFace(fontDesc)); // There is already a font added with this description!

	TBFontInfo *fi = getFontInfo(fontDesc.getID());
	if (fi == nullptr) {
		return nullptr;
	}

	if (fi->getID() == 0) // Is this the test dummy font
	{
		if (TBFontFace *font = new TBFontFace(&m_glyph_cache, nullptr, fontDesc)) {
			if (m_fonts.add(fontDesc.getFontFaceID(), font)) {
				return font;
			}
			delete font;
		}
		return nullptr;
	}

	// Iterate through font renderers until we find one capable of creating a font for this file.
	for (TBFontRenderer *fr = m_font_renderers.getFirst(); fr != nullptr; fr = fr->getNext()) {
		if (TBFontFace *font = fr->create(this, fi->getFilename(), fontDesc)) {
			if (m_fonts.add(fontDesc.getFontFaceID(), font)) {
				return font;
			}
			delete font;
		}
	}
	return nullptr;
}

} // namespace tb
