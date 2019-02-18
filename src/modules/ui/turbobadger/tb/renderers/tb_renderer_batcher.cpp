/**
 * @file
 */

#include "renderers/tb_renderer_batcher.h"
#include "tb_bitmap_fragment.h"
#include "tb_system.h"

namespace tb {

// == TBRendererBatcher::Batch ==========================================================

#ifdef TB_RUNTIME_DEBUG_INFO
uint32_t dbg_begin_paint_batch_id = 0;
uint32_t dbg_frame_triangle_count = 0;
#endif // TB_RUNTIME_DEBUG_INFO

#define VER_COL(r, g, b, a) (((a) << 24) + ((b) << 16) + ((g) << 8) + r)
#define VER_COL_OPACITY(a) (0x00ffffff + (((uint32_t)a) << 24))

void TBRendererBatcher::Batch::flush(TBRendererBatcher *batchRenderer) {
	if (!vertex_count || is_flushing)
		return;

	// Prevent re-entrancy. Calling fragment->getBitmap may end up calling TBBitmap::setData
	// which will end up flushing any existing batch with that bitmap.
	is_flushing = true;

	if (fragment) {
		// Now it's time to ensure the bitmap data is up to date. A call to GetBitmap
		// with TB_VALIDATE_ALWAYS should guarantee that its data is validated.
		TBBitmap *frag_bitmap = fragment->getBitmap(TB_VALIDATE_ALWAYS);
		((void)frag_bitmap); // silence warning about unused variable
		core_assert(frag_bitmap == bitmap);
	}

	batchRenderer->renderBatch(this);

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES)) {
		// This assumes we're drawing triangles. Need to modify this
		// if we start using strips, fans or whatever.
		dbg_frame_triangle_count += vertex_count / 3;

		// Draw the triangles again using a random color based on the batch
		// id. This indicates which triangles belong to the same batch.
		uint32_t id = batch_id - dbg_begin_paint_batch_id;
		uint32_t hash = id * (2166136261U ^ id);
		uint32_t color = 0xAA000000 + (hash & 0x00FFFFFF);
		for (int i = 0; i < vertex_count; i++)
			vertex[i].col = color;
		bitmap = nullptr;
		batchRenderer->renderBatch(this);
	}
#endif // TB_RUNTIME_DEBUG_INFO

	vertex_count = 0;

	batch_id++; // Will overflow eventually, but that doesn't really matter.

	is_flushing = false;
}

TBRendererBatcher::Vertex *TBRendererBatcher::Batch::reserve(TBRendererBatcher *batchRenderer, int count) {
	core_assert(count < VERTEX_BATCH_SIZE);
	if (vertex_count + count > VERTEX_BATCH_SIZE)
		flush(batchRenderer);
	Vertex *ret = &vertex[vertex_count];
	vertex_count += count;
	return ret;
}

// == TBRendererBatcher ===================================================================

TBRendererBatcher::TBRendererBatcher()
	: m_opacity(255), m_translation_x(0), m_translation_y(0), m_u(0), m_v(0), m_uu(0), m_vv(0) {
}

TBRendererBatcher::~TBRendererBatcher() {
}

void TBRendererBatcher::beginPaint(int renderTargetW, int renderTargetH) {
#ifdef TB_RUNTIME_DEBUG_INFO
	dbg_begin_paint_batch_id = batch.batch_id;
	dbg_frame_triangle_count = 0;
#endif // TB_RUNTIME_DEBUG_INFO

	m_screen_rect.set(0, 0, renderTargetW, renderTargetH);
	m_clip_rect = m_screen_rect;
}

void TBRendererBatcher::endPaint() {
	flushAllInternal();

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES))
		Log::debug("Frame rendered using %d batches and a total of %d triangles.",
				   batch.batch_id - dbg_begin_paint_batch_id, dbg_frame_triangle_count);
#endif // TB_RUNTIME_DEBUG_INFO
}

void TBRendererBatcher::translate(int dx, int dy) {
	m_translation_x += dx;
	m_translation_y += dy;
}

void TBRendererBatcher::setOpacity(float opacity) {
	int8_t opacity8 = (uint8_t)(opacity * 255);
	if (opacity8 == m_opacity)
		return;
	m_opacity = opacity8;
}

float TBRendererBatcher::getOpacity() {
	return m_opacity / 255.f;
}

TBRect TBRendererBatcher::setClipRect(const TBRect &rect, bool addToCurrent) {
	TBRect old_clip_rect = m_clip_rect;
	m_clip_rect = rect;
	m_clip_rect.x += m_translation_x;
	m_clip_rect.y += m_translation_y;

	if (addToCurrent)
		m_clip_rect = m_clip_rect.clip(old_clip_rect);

	flushAllInternal();
	setClipRect(m_clip_rect);

	old_clip_rect.x -= m_translation_x;
	old_clip_rect.y -= m_translation_y;
	return old_clip_rect;
}

TBRect TBRendererBatcher::getClipRect() {
	TBRect curr_clip_rect = m_clip_rect;
	curr_clip_rect.x -= m_translation_x;
	curr_clip_rect.y -= m_translation_y;
	return curr_clip_rect;
}

void TBRendererBatcher::drawBitmap(const TBRect &dstRect, const TBRect &srcRect, TBBitmapFragment *bitmapFragment) {
	if (TBBitmap *bitmap = bitmapFragment->getBitmap(TB_VALIDATE_FIRST_TIME))
		addQuadInternal(dstRect.offset(m_translation_x, m_translation_y),
						srcRect.offset(bitmapFragment->m_rect.x, bitmapFragment->m_rect.y), VER_COL_OPACITY(m_opacity),
						bitmap, bitmapFragment);
}

void TBRendererBatcher::drawBitmap(const TBRect &dstRect, const TBRect &srcRect, TBBitmap *bitmap) {
	addQuadInternal(dstRect.offset(m_translation_x, m_translation_y), srcRect, VER_COL_OPACITY(m_opacity), bitmap,
					nullptr);
}

void TBRendererBatcher::drawBitmapColored(const TBRect &dstRect, const TBRect &srcRect, const TBColor &color,
										  TBBitmapFragment *bitmapFragment) {
	if (TBBitmap *bitmap = bitmapFragment->getBitmap(TB_VALIDATE_FIRST_TIME)) {
		uint32_t a = (color.a * m_opacity) / 255;
		addQuadInternal(dstRect.offset(m_translation_x, m_translation_y),
						srcRect.offset(bitmapFragment->m_rect.x, bitmapFragment->m_rect.y),
						VER_COL(color.r, color.g, color.b, a), bitmap, bitmapFragment);
	}
}

void TBRendererBatcher::drawBitmapColored(const TBRect &dstRect, const TBRect &srcRect, const TBColor &color,
										  TBBitmap *bitmap) {
	uint32_t a = (color.a * m_opacity) / 255;
	addQuadInternal(dstRect.offset(m_translation_x, m_translation_y), srcRect, VER_COL(color.r, color.g, color.b, a),
					bitmap, nullptr);
}

void TBRendererBatcher::drawBitmapTile(const TBRect &dstRect, TBBitmap *bitmap) {
	addQuadInternal(dstRect.offset(m_translation_x, m_translation_y), TBRect(0, 0, dstRect.w, dstRect.h),
					VER_COL_OPACITY(m_opacity), bitmap, nullptr);
}

void TBRendererBatcher::addQuadInternal(const TBRect &dstRect, const TBRect &srcRect, uint32_t color, TBBitmap *bitmap,
										TBBitmapFragment *fragment) {
	if (batch.bitmap != bitmap) {
		batch.flush(this);
		batch.bitmap = bitmap;
	}
	batch.fragment = fragment;

	const int bitmap_w = bitmap->width();
	const int bitmap_h = bitmap->height();
	m_u = (float)srcRect.x / bitmap_w;
	m_v = (float)srcRect.y / bitmap_h;
	m_uu = (float)(srcRect.x + srcRect.w) / bitmap_w;
	m_vv = (float)(srcRect.y + srcRect.h) / bitmap_h;

	Vertex *ver = batch.reserve(this, 6);
	ver[0].x = (float)dstRect.x;
	ver[0].y = (float)(dstRect.y + dstRect.h);
	ver[0].u = m_u;
	ver[0].v = m_vv;
	ver[0].col = color;
	ver[1].x = (float)(dstRect.x + dstRect.w);
	ver[1].y = (float)(dstRect.y + dstRect.h);
	ver[1].u = m_uu;
	ver[1].v = m_vv;
	ver[1].col = color;
	ver[2].x = (float)dstRect.x;
	ver[2].y = (float)dstRect.y;
	ver[2].u = m_u;
	ver[2].v = m_v;
	ver[2].col = color;

	ver[3].x = (float)dstRect.x;
	ver[3].y = (float)dstRect.y;
	ver[3].u = m_u;
	ver[3].v = m_v;
	ver[3].col = color;
	ver[4].x = (float)(dstRect.x + dstRect.w);
	ver[4].y = (float)(dstRect.y + dstRect.h);
	ver[4].u = m_uu;
	ver[4].v = m_vv;
	ver[4].col = color;
	ver[5].x = (float)(dstRect.x + dstRect.w);
	ver[5].y = (float)dstRect.y;
	ver[5].u = m_uu;
	ver[5].v = m_v;
	ver[5].col = color;

	// Update fragments batch id (See FlushBitmapFragment)
	if (fragment)
		fragment->m_batch_id = batch.batch_id;
}

void TBRendererBatcher::flushAllInternal() {
	batch.flush(this);
}

void TBRendererBatcher::flushBitmap(TBBitmap *bitmap) {
	// Flush the batch if it's using this bitmap (that is about to change or be deleted)
	if (batch.vertex_count && bitmap == batch.bitmap)
		batch.flush(this);
}

void TBRendererBatcher::flushBitmapFragment(TBBitmapFragment *bitmapFragment) {
	// Flush the batch if it is using this fragment (that is about to change or be deleted)
	// We know if it is in use in the current batch if its batch_id matches the current
	// batch_id in our (one and only) batch.
	// If we switch to a more advance batching system with multiple batches, we need to
	// solve this a bit differently.
	if (batch.vertex_count && bitmapFragment->m_batch_id == batch.batch_id)
		batch.flush(this);
}

} // namespace tb
