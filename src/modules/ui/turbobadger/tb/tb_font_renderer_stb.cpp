/**
 * @file
 */

#include "core/Assert.h"
#include "tb_font_renderer.h"
#include "tb_renderer.h"
#include "tb_system.h"
#include "tb_tempbuffer.h"

using namespace tb;

#define STBTT_assert core_assert
#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "thirdparty/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION

/** STBFontRenderer renders fonts using stb_truetype.h (http://nothings.org/) */

class STBFontRenderer : public TBFontRenderer {
public:
	STBFontRenderer();
	~STBFontRenderer();

	bool load(const char *filename, int size);

	virtual TBFontFace *create(TBFontManager *fontManager, const char *filename, const TBFontDescription &fontDesc);

	virtual TBFontMetrics getMetrics();
	virtual bool renderGlyph(TBFontGlyphData *data, UCS4 cp);
	virtual void getGlyphMetrics(TBGlyphMetrics *metrics, UCS4 cp);

private:
	stbtt_fontinfo font;
	TBTempBuffer ttf_buffer;
	unsigned char *render_data = nullptr;
	int font_size = 0;
	float scale = 0.0F;
};

STBFontRenderer::STBFontRenderer() {
}

STBFontRenderer::~STBFontRenderer() {
	delete[] render_data;
}

TBFontMetrics STBFontRenderer::getMetrics() {
	TBFontMetrics metrics;
	int ascent;
	int descent;
	int lineGap;
	stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
	metrics.ascent = (int)(ascent * scale + 0.5F);
	metrics.descent = (int)((-descent) * scale + 0.5F);
	metrics.height = (int)((ascent - descent + lineGap) * scale + 0.5F);
	return metrics;
}

bool STBFontRenderer::renderGlyph(TBFontGlyphData *data, UCS4 cp) {
	delete[] render_data;
	render_data = stbtt_GetCodepointBitmap(&font, 0, scale, cp, &data->w, &data->h, 0, 0);
	data->data8 = render_data;
	data->stride = data->w;
	data->rgb = false;
	return data->data8 != nullptr;
}

void STBFontRenderer::getGlyphMetrics(TBGlyphMetrics *metrics, UCS4 cp) {
	int advanceWidth;
	int leftSideBearing;
	stbtt_GetCodepointHMetrics(&font, cp, &advanceWidth, &leftSideBearing);
	metrics->advance = (int)(advanceWidth * scale + 0.5F);
	int ix0;
	int iy0;
	int ix1;
	int iy1;
	stbtt_GetCodepointBitmapBox(&font, cp, 0, scale, &ix0, &iy0, &ix1, &iy1);
	metrics->x = ix0;
	metrics->y = iy0;
}

bool STBFontRenderer::load(const char *filename, int size) {
	if (!ttf_buffer.appendFile(filename)) {
		return false;
	}

	const unsigned char *ttf_ptr = (const unsigned char *)ttf_buffer.getData();
	stbtt_InitFont(&font, ttf_ptr, stbtt_GetFontOffsetForIndex(ttf_ptr, 0));

	font_size = (int)(size * 1.3F); // FIX: Constant taken out of thin air because fonts get too small.
	scale = stbtt_ScaleForPixelHeight(&font, (float)font_size);
	return true;
}

TBFontFace *STBFontRenderer::create(TBFontManager *fontManager, const char *filename,
									const TBFontDescription &fontDesc) {
	if (STBFontRenderer *fr = new STBFontRenderer()) {
		if (fr->load(filename, (int)fontDesc.getSize())) {
			if (TBFontFace *font = new TBFontFace(fontManager->getGlyphCache(), fr, fontDesc)) {
				return font;
			}
		}
		delete fr;
	}
	return nullptr;
}

void register_stb_font_renderer() {
	if (STBFontRenderer *fr = new STBFontRenderer) {
		g_font_manager->addRenderer(fr);
	}
}
